/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <clang/AST/ASTContext.h>
#if LLVM_VERSION_MAJOR >= 11
#include <clang/AST/ParentMapContext.h>
#endif
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <future>
#include <iterator>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include "sentinel/Mutants.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/WeightedMutantGenerator.hpp"
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/operators/MutationOperator.hpp"

namespace sentinel {

namespace fs = std::filesystem;

WeightedMutantGenerator::WeightedMutantGenerator(const std::filesystem::path& path) : MutantGenerator(path) {
}

// ---------------------------------------------------------------------------
// collectAllMutants — overridden to use DepthAware AST visitor
// ---------------------------------------------------------------------------
Mutants WeightedMutantGenerator::collectAllMutants(const SourceLines& sourceLines) {
  Mutants mutables;
  mDepthMap.clear();
  std::string errorMsg;
  std::unique_ptr<clang::tooling::CompilationDatabase> compileDb =
      clang::tooling::CompilationDatabase::loadFromDirectory(mDbPath.string(), errorMsg);

  if (compileDb == nullptr) {
    throw IOException(EINVAL, errorMsg);
  }

  std::map<fs::path, SourceLines> targetLines;
  DepthMap depthMap;

  for (const auto& sourceLine : sourceLines) {
    fs::path filename = sourceLine.getPath();
    targetLines[filename].push_back(sourceLine);
  }

  using FileResult = std::pair<Mutants, DepthMap>;
  unsigned int maxThreads = std::max(1u, std::thread::hardware_concurrency());
  auto* db = compileDb.get();

  auto fileIt = targetLines.begin();
  while (fileIt != targetLines.end()) {
    std::vector<std::future<FileResult>> futures;
    for (unsigned int i = 0; i < maxThreads && fileIt != targetLines.end(); ++i, ++fileIt) {
      DepthMap localDm;
      for (const auto& sl : fileIt->second) {
        localDm[sl] = -1;
      }
      futures.push_back(std::async(std::launch::async, [db, filename = fileIt->first, fileLines = fileIt->second,
                                                        ldm = std::move(localDm), this]() mutable {
        try {
          Mutants localMutables;
          auto factory = createDepthAwareActionFactory(&localMutables, fileLines, &ldm, mSelectedOperators);
          runClangToolForFile(*db, filename, factory.get());
          return std::make_pair(std::move(localMutables), std::move(ldm));
        } catch (const std::bad_alloc&) {
          rethrowAsOomError(filename);
        }
      }));
    }
    for (auto& fut : futures) {
      auto result = fut.get();
      mutables.insert(mutables.end(), std::make_move_iterator(result.first.begin()),
                      std::make_move_iterator(result.first.end()));
      depthMap.insert(result.second.begin(), result.second.end());
    }
  }

  mDepthMap = std::move(depthMap);
  return mutables;
}

// ---------------------------------------------------------------------------
// selectMutants — depth-ordered selection
// ---------------------------------------------------------------------------
Mutants WeightedMutantGenerator::selectMutants(const SourceLines& sourceLines, std::size_t maxMutants,
                                               unsigned int randomSeed, const CandidateIndex& index,
                                               std::size_t mutantsPerLine) {
  auto cmp = [](const std::pair<SourceLine, int>& a, const std::pair<SourceLine, int>& b) {
    return a.second > b.second;
  };

  std::vector<std::pair<SourceLine, int>> sortedDepthMap;
  sortedDepthMap.reserve(mDepthMap.size());
  std::copy(mDepthMap.begin(), mDepthMap.end(), std::back_inserter(sortedDepthMap));
  std::sort(sortedDepthMap.begin(), sortedDepthMap.end(), cmp);

  return selectFromRange(sortedDepthMap, maxMutants, randomSeed, index,
                         [](const std::pair<SourceLine, int>& elem) -> const SourceLine& { return elem.first; },
                         mutantsPerLine);
}

// ---------------------------------------------------------------------------
// DepthAwareASTVisitor
// ---------------------------------------------------------------------------
WeightedMutantGenerator::DepthAwareASTVisitor::DepthAwareASTVisitor(
    clang::ASTContext* context, Mutants* mutables, const SourceLines& targetLines,
    DepthMap* depthMap, const std::vector<std::string>& selectedOps) :
    mContext(context),
    mSrcMgr(context->getSourceManager()),
    mMutationOperators(createOperators(context, selectedOps)),
    mMutants(mutables),
    mTargetLines(targetLines),
    mDepthMap(depthMap) {
}

WeightedMutantGenerator::DepthAwareASTVisitor::~DepthAwareASTVisitor() = default;

bool WeightedMutantGenerator::DepthAwareASTVisitor::VisitStmt(clang::Stmt* s) {
  std::size_t startLineNum = 0;
  std::size_t endLineNum = 0;
  resolveExpansionLineRange(s, &mSrcMgr, mContext->getLangOpts(),
                            &startLineNum, &endLineNum);
  bool containTargetLine = false;
  for (const auto& line : mTargetLines) {
    std::size_t lineNum = line.getLineNumber();
    if (lineNum >= startLineNum && lineNum <= endLineNum) {
      containTargetLine = true;
      int depth = getDepth(s);
      if ((*mDepthMap)[line] < depth) {
        (*mDepthMap)[line] = depth;
      }
    }
  }

  if (containTargetLine) {
    for (const auto& m : mMutationOperators) {
      if (m->canMutate(s)) {
        m->populate(s, mMutants);
      }
    }
  }

  return true;
}

int WeightedMutantGenerator::DepthAwareASTVisitor::getDepth(clang::Stmt* s) {
  const clang::Stmt* stmt = s;
  const clang::Decl* decl = nullptr;
  auto parents = mContext->getParents(*s);
  int depth = 0;
  while (true) {
    if (stmt != nullptr) {
      parents = mContext->getParents(*stmt);
    }

    if (decl != nullptr) {
      parents = mContext->getParents(*decl);
    }

    if (parents.empty()) {
      return depth;
    }

    const auto stmtParent = parents[0].get<clang::Stmt>();
    const auto declParent = parents[0].get<clang::Decl>();
    if (stmtParent == nullptr) {
      if (declParent == nullptr) {
        break;
      }

      stmt = nullptr;
      decl = declParent;
      if (clang::isa<clang::FunctionDecl>(decl)) {
        return depth;
      }
    } else {
      if (clang::isa<clang::CompoundStmt>(stmtParent)) {
        depth++;
      }
      stmt = stmtParent;
      decl = nullptr;
    }
  }

  return depth;
}

// ---------------------------------------------------------------------------
// DepthAwareASTConsumer
// ---------------------------------------------------------------------------
WeightedMutantGenerator::DepthAwareASTConsumer::DepthAwareASTConsumer(
    const clang::CompilerInstance& ci, Mutants* mutables, const SourceLines& targetLines,
    DepthMap* depthMap, const std::vector<std::string>& selectedOps) :
    mMutants(mutables), mTargetLines(targetLines), mDepthMap(depthMap), mSelectedOps(selectedOps) {
}

void WeightedMutantGenerator::DepthAwareASTConsumer::HandleTranslationUnit(clang::ASTContext& context) {
  DepthAwareASTVisitor visitor(&context, mMutants, mTargetLines, mDepthMap, mSelectedOps);
  visitor.TraverseDecl(context.getTranslationUnitDecl());
}

// ---------------------------------------------------------------------------
// DepthAwareAction
// ---------------------------------------------------------------------------
WeightedMutantGenerator::DepthAwareAction::DepthAwareAction(
    Mutants* mutables, const SourceLines& targetLines, DepthMap* depthMap,
    const std::vector<std::string>& selectedOps) :
    mMutants(mutables), mTargetLines(targetLines), mDepthMap(depthMap), mSelectedOps(selectedOps) {
}

std::unique_ptr<clang::ASTConsumer> WeightedMutantGenerator::DepthAwareAction::CreateASTConsumer(
    clang::CompilerInstance& ci, llvm::StringRef inFile) {
  ci.getDiagnostics().setClient(new clang::IgnoringDiagConsumer());
  return std::unique_ptr<clang::ASTConsumer>(
      new DepthAwareASTConsumer(ci, mMutants, mTargetLines, mDepthMap, mSelectedOps));
}

// ---------------------------------------------------------------------------
// createDepthAwareActionFactory
// ---------------------------------------------------------------------------
std::unique_ptr<clang::tooling::FrontendActionFactory> WeightedMutantGenerator::createDepthAwareActionFactory(
    Mutants* mutables, const SourceLines& targetLines, DepthMap* depthMap,
    const std::vector<std::string>& selectedOps) {
  class DepthAwareFrontendActionFactory : public clang::tooling::FrontendActionFactory {
   public:
    DepthAwareFrontendActionFactory(Mutants* mutables, const SourceLines& targetLines,
                                    DepthMap* depthMap, const std::vector<std::string>& selectedOps) :
        mMutants(mutables), mTargetLines(targetLines), mDepthMap(depthMap), mSelectedOps(selectedOps) {
    }

#if LLVM_VERSION_MAJOR >= 10
    std::unique_ptr<clang::FrontendAction> create() override {
      return std::make_unique<DepthAwareAction>(mMutants, mTargetLines, mDepthMap, mSelectedOps);
    }
#else
    clang::FrontendAction* create() override {
      return new DepthAwareAction(mMutants, mTargetLines, mDepthMap, mSelectedOps);
    }
#endif

   private:
    Mutants* mMutants;
    const SourceLines& mTargetLines;
    DepthMap* mDepthMap;
    const std::vector<std::string>& mSelectedOps;
  };

  return std::unique_ptr<clang::tooling::FrontendActionFactory>(
      new DepthAwareFrontendActionFactory(mutables, targetLines, depthMap, selectedOps));
}

}  // namespace sentinel
