/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <clang/AST/ASTContext.h>
#if LLVM_VERSION_MAJOR >= 11
#include <clang/AST/ParentMapContext.h>
#endif
#include <clang/Lex/Lexer.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <future>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include "sentinel/Logger.hpp"
#include "sentinel/Mutants.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/WeightedMutantGenerator.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace fs = std::filesystem;

namespace sentinel {

static const char* cWeightedGeneratorLoggerName = "WeightedMutantGenerator";

WeightedMutantGenerator::WeightedMutantGenerator(const std::filesystem::path& path) : mDbPath(path) {
}

Mutants WeightedMutantGenerator::populate(const SourceLines& sourceLines, std::size_t maxMutants, unsigned randomSeed) {
  Mutants mutables;
  std::string errorMsg;
  std::unique_ptr<clang::tooling::CompilationDatabase> compileDb =
      clang::tooling::CompilationDatabase::loadFromDirectory(mDbPath.string(), errorMsg);

  if (compileDb == nullptr) {
    throw IOException(EINVAL, errorMsg);
  }

  // convert sourceLines to map from filename to list of target source lines
  std::map<fs::path, SourceLines> targetLines;
  std::map<SourceLine, int> depthMap;

  for (const auto& sourceLine : sourceLines) {
    fs::path filename = sourceLine.getPath();
    targetLines[filename].push_back(sourceLine);
  }

  auto logger = Logger::getLogger(cWeightedGeneratorLoggerName);
  // Launch async tasks in batches capped at hardware_concurrency to avoid overloading the system.
  // Each task receives its own per-file DepthMap slice; results are merged after all tasks finish.
  // ClangTool::run() calls chdir() internally (process-wide). Save cwd before launching tasks
  // and restore it after all tasks complete to neutralise the side effect.
  using FileResult = std::pair<Mutants, DepthMap>;
  unsigned int maxThreads = std::max(1u, std::thread::hardware_concurrency());
  auto savedCwd = fs::current_path();
  auto* db = compileDb.get();

  auto fileIt = targetLines.begin();
  while (fileIt != targetLines.end()) {
    std::vector<std::future<FileResult>> futures;
    for (unsigned int i = 0; i < maxThreads && fileIt != targetLines.end(); ++i, ++fileIt) {
      logger->verbose("Checking for mutants in {}", fileIt->first.string());
      DepthMap localDm;
      for (const auto& sl : fileIt->second) {
        localDm[sl] = -1;
      }
      futures.push_back(std::async(
          std::launch::async,
          [db, filename = fileIt->first, fileLines = fileIt->second, ldm = std::move(localDm), this]() mutable {
            Mutants localMutables;
            clang::tooling::ClangTool tool(*db, filename.string());
            tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster("-ferror-limit=0"));
            tool.run(myNewFrontendActionFactory(&localMutables, fileLines, &ldm, mSelectedOperators).get());
            return std::make_pair(std::move(localMutables), std::move(ldm));
          }));
    }
    for (auto& fut : futures) {
      auto result = fut.get();
      std::copy(result.first.begin(), result.first.end(), std::back_inserter(mutables));
      depthMap.insert(result.second.begin(), result.second.end());
    }
  }
  fs::current_path(savedCwd);

  // Sort the depthMap in order of descending depth.
  auto cmp = [](const std::pair<SourceLine, int>& a, const std::pair<SourceLine, int>& b) {
    return a.second > b.second;
  };

  std::vector<std::pair<SourceLine, int>> sortedDepthMap;
  sortedDepthMap.reserve(depthMap.size());
  std::copy(depthMap.begin(), depthMap.end(), std::back_inserter(sortedDepthMap));
  std::sort(sortedDepthMap.begin(), sortedDepthMap.end(), cmp);

  // Pre-group mutants by canonical file path, sorted by first.line for binary search.
  // Mutant::mPath is already canonical (set in Mutant constructor).
  std::map<fs::path, std::vector<const Mutant*>> mutantsByFile;
  for (const auto& m : mutables) {
    mutantsByFile[m.getPath()].push_back(&m);
  }
  for (auto& entry : mutantsByFile) {
    std::sort(entry.second.begin(), entry.second.end(), [](const Mutant* a, const Mutant* b) {
      return a->getFirst().line < b->getFirst().line;
    });
  }

  // Cache canonical path per unique source path to avoid repeated fs::canonical() calls.
  std::map<fs::path, fs::path> pathCache;

  // Select one Mutant on each target line
  std::set<Mutant> selectedSet;
  Mutants temp_storage;
  std::mt19937 rng(randomSeed);
  for (const auto& it : sortedDepthMap) {
    auto line = it.first;

    fs::path rawPath = line.getPath();
    auto emplaceResult = pathCache.emplace(rawPath, fs::path{});
    if (emplaceResult.second) {
      emplaceResult.first->second = fs::canonical(rawPath);
    }
    const fs::path& canonPath = emplaceResult.first->second;
    std::size_t targetLine = line.getLineNumber();

    auto mutantsByFileIt = mutantsByFile.find(canonPath);
    if (mutantsByFileIt == mutantsByFile.end()) {
      continue;
    }

    const auto& fileVec = mutantsByFileIt->second;

    // Binary search: upper_bound finds end of mutants with first.line <= targetLine
    auto endIt = std::upper_bound(fileVec.begin(), fileVec.end(), targetLine,
        [](std::size_t t, const Mutant* m) { return t < m->getFirst().line; });

    // Filter: only keep mutants where last.line >= targetLine
    std::vector<const Mutant*> candidates;
    std::copy_if(fileVec.begin(), endIt, std::back_inserter(candidates),
        [targetLine](const Mutant* m) { return m->getLast().line >= targetLine; });

    // Continue if there are no generatable mutants.
    if (candidates.empty()) {
      continue;
    }

    // Randomly select one mutant.
    std::shuffle(candidates.begin(), candidates.end(), rng);

    // find first candidate not already in selectedSet
    for (const auto* candidate : candidates) {
      if (selectedSet.insert(*candidate).second) {
        temp_storage.push_back(*candidate);
        break;
      }
    }

    // Break if maximum number of mutants is reached.
    if (temp_storage.size() == maxMutants) {
      break;
    }
  }

  mCandidateCount = mutables.size();
  return temp_storage;
}

WeightedMutantGenerator::SentinelASTVisitor::SentinelASTVisitor(clang::ASTContext* Context, Mutants* mutables,
                                                                const SourceLines& targetLines, DepthMap* depthMap,
                                                                const std::vector<std::string>& selectedOps) :
    mContext(Context),
    mSrcMgr(Context->getSourceManager()),
    mMutants(mutables),
    mTargetLines(targetLines),
    mDepthMap(depthMap) {
  auto include = [&selectedOps](const std::string& name) {
    return selectedOps.empty() || std::find(selectedOps.begin(), selectedOps.end(), name) != selectedOps.end();
  };
  if (include("AOR")) mMutationOperators.push_back(std::make_unique<AOR>(Context));
  if (include("BOR")) mMutationOperators.push_back(std::make_unique<BOR>(Context));
  if (include("ROR")) mMutationOperators.push_back(std::make_unique<ROR>(Context));
  if (include("SOR")) mMutationOperators.push_back(std::make_unique<SOR>(Context));
  if (include("LCR")) mMutationOperators.push_back(std::make_unique<LCR>(Context));
  if (include("SDL")) mMutationOperators.push_back(std::make_unique<SDL>(Context));
  if (include("UOI")) mMutationOperators.push_back(std::make_unique<UOI>(Context));
}

WeightedMutantGenerator::SentinelASTVisitor::~SentinelASTVisitor() = default;

bool WeightedMutantGenerator::SentinelASTVisitor::VisitStmt(clang::Stmt* s) {
  clang::SourceLocation startLoc = s->getBeginLoc();
  clang::SourceLocation endLoc = s->getEndLoc();

  // Check if this Stmt node represents code on target lines.
  if (startLoc.isMacroID()) {
    clang::CharSourceRange range = mContext->getSourceManager().getImmediateExpansionRange(startLoc);
    startLoc = range.getBegin();
  }

  if (endLoc.isMacroID()) {
    clang::CharSourceRange range = mContext->getSourceManager().getImmediateExpansionRange(endLoc);
    endLoc =
        clang::Lexer::getLocForEndOfToken(range.getEnd(), 0, mContext->getSourceManager(), mContext->getLangOpts());
  }

  std::size_t startLineNum = mSrcMgr.getExpansionLineNumber(startLoc);
  std::size_t endLineNum = mSrcMgr.getExpansionLineNumber(endLoc);
  bool containTargetLine =
      std::any_of(mTargetLines.begin(), mTargetLines.end(), [startLineNum, endLineNum, s, this](SourceLine line) {
        int lineNum = line.getLineNumber();
        bool ret = lineNum >= startLineNum && lineNum <= endLineNum;
        if (ret) {
          int temp = getDepth(s);
          if ((*mDepthMap)[line] < temp) {
            (*mDepthMap)[line] = temp;
          }
        }
        return ret;
      });

  if (containTargetLine) {
    for (const auto& m : mMutationOperators) {
      if (m->canMutate(s)) {
        m->populate(s, mMutants);
      }
    }
  }

  return true;
}

int WeightedMutantGenerator::SentinelASTVisitor::getDepth(clang::Stmt* s) {
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

WeightedMutantGenerator::SentinelASTConsumer::SentinelASTConsumer(const clang::CompilerInstance& CI,
                                                                  Mutants* mutables,
                                                                  const SourceLines& targetLines,
                                                                  DepthMap* depthMap,
                                                                  const std::vector<std::string>& selectedOps) :
    mMutants(mutables), mTargetLines(targetLines), mDepthMap(depthMap), mSelectedOps(selectedOps) {
}

void WeightedMutantGenerator::SentinelASTConsumer::HandleTranslationUnit(clang::ASTContext& Context) {
  SentinelASTVisitor mVisitor(&Context, mMutants, mTargetLines, mDepthMap, mSelectedOps);
  mVisitor.TraverseDecl(Context.getTranslationUnitDecl());
}

WeightedMutantGenerator::GenerateMutantAction::GenerateMutantAction(Mutants* mutables,
                                                                    const SourceLines& targetLines,
                                                                    DepthMap* depthMap,
                                                                    const std::vector<std::string>& selectedOps) :
    mMutants(mutables), mTargetLines(targetLines), mDepthMap(depthMap), mSelectedOps(selectedOps) {
}

std::unique_ptr<clang::ASTConsumer> WeightedMutantGenerator::GenerateMutantAction::CreateASTConsumer(
    clang::CompilerInstance& CI, llvm::StringRef InFile) {
  CI.getDiagnostics().setClient(new clang::IgnoringDiagConsumer());
  return std::unique_ptr<clang::ASTConsumer>(
      new SentinelASTConsumer(CI, mMutants, mTargetLines, mDepthMap, mSelectedOps));
}

void WeightedMutantGenerator::GenerateMutantAction::ExecuteAction() {
  // AST Traversal.
  clang::ASTFrontendAction::ExecuteAction();
}

std::unique_ptr<clang::tooling::FrontendActionFactory> WeightedMutantGenerator::myNewFrontendActionFactory(
    Mutants* mutables, const SourceLines& targetLines, DepthMap* depthMap,
    const std::vector<std::string>& selectedOps) {
  class SimpleFrontendActionFactory : public clang::tooling::FrontendActionFactory {
   public:
    SimpleFrontendActionFactory(Mutants* mutables, const SourceLines& targetLines, DepthMap* depthMap,
                                const std::vector<std::string>& selectedOps) :
        mMutants(mutables), mTargetLines(targetLines), mDepthMap(depthMap), mSelectedOps(selectedOps) {}

#if LLVM_VERSION_MAJOR >= 10
    std::unique_ptr<clang::FrontendAction> create() override {
      return std::make_unique<GenerateMutantAction>(mMutants, mTargetLines, mDepthMap, mSelectedOps);
    }
#else
    clang::FrontendAction* create() override {
      return new GenerateMutantAction(mMutants, mTargetLines, mDepthMap, mSelectedOps);
    }
#endif

   private:
    Mutants* mMutants;
    const SourceLines& mTargetLines;
    DepthMap* mDepthMap;
    const std::vector<std::string>& mSelectedOps;
  };

  return std::unique_ptr<clang::tooling::FrontendActionFactory>(
      new SimpleFrontendActionFactory(mutables, targetLines, depthMap, selectedOps));
}

}  // namespace sentinel
