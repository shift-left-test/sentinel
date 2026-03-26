/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

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
#include "sentinel/Mutants.hpp"
#include "sentinel/RandomMutantGenerator.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace sentinel {

namespace fs = std::filesystem;

RandomMutantGenerator::RandomMutantGenerator(const std::filesystem::path& path) : mDbPath(path) {
}

Mutants RandomMutantGenerator::generate(const SourceLines& sourceLines, std::size_t maxMutants,
                                        unsigned int randomSeed) {
  Mutants mutables;
  std::string errorMsg;
  std::unique_ptr<clang::tooling::CompilationDatabase> compileDb =
      clang::tooling::CompilationDatabase::loadFromDirectory(mDbPath.string(), errorMsg);

  if (compileDb == nullptr) {
    throw IOException(EINVAL, errorMsg);
  }

  // convert sourceLines to map from filename to list of target source lines
  std::map<fs::path, std::vector<std::size_t>> targetLines;

  for (const auto& sourceLine : sourceLines) {
    fs::path filename = sourceLine.getPath();
    targetLines[filename].push_back(sourceLine.getLineNumber());
  }

  // Launch async tasks in batches capped at hardware_concurrency to avoid overloading the system.
  // ClangTool::run() calls chdir() internally (process-wide). Save cwd before launching tasks
  // and restore it after all tasks complete to neutralise the side effect.
  unsigned int maxThreads = std::max(1u, std::thread::hardware_concurrency());
  auto savedCwd = fs::current_path();
  auto* db = compileDb.get();

  auto fileIt = targetLines.begin();
  while (fileIt != targetLines.end()) {
    std::vector<std::future<Mutants>> futures;
    for (unsigned int i = 0; i < maxThreads && fileIt != targetLines.end(); ++i, ++fileIt) {
      futures.push_back(std::async(std::launch::async, [db, filename = fileIt->first, lines = fileIt->second, this]() {
        Mutants localMutables;
        clang::tooling::ClangTool tool(*db, filename.string());
        tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster("-ferror-limit=0"));
        tool.run(myNewFrontendActionFactory(&localMutables, lines, mSelectedOperators).get());
        return localMutables;
      }));
    }
    for (auto& fut : futures) {
      auto localMutables = fut.get();
      std::copy(localMutables.begin(), localMutables.end(), std::back_inserter(mutables));
    }
  }
  fs::current_path(savedCwd);

  // Build candidates pool: one mutant per source line (consistent with Uniform/Weighted strategy).
  std::map<fs::path, std::vector<const Mutant*>> mutantsByFile;
  for (const auto& m : mutables) {
    mutantsByFile[m.getPath()].push_back(&m);
  }
  for (auto& entry : mutantsByFile) {
    std::sort(entry.second.begin(), entry.second.end(),
              [](const Mutant* a, const Mutant* b) { return a->getFirst().line < b->getFirst().line; });
  }

  std::mt19937 rng(randomSeed);
  std::map<fs::path, fs::path> pathCache;
  std::set<Mutant> selectedSet;
  Mutants candidates;

  for (const auto& line : sourceLines) {
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

    auto endIt = std::upper_bound(fileVec.begin(), fileVec.end(), targetLine,
                                  [](std::size_t t, const Mutant* m) { return t < m->getFirst().line; });
    std::vector<const Mutant*> lineCandidates;
    std::copy_if(fileVec.begin(), endIt, std::back_inserter(lineCandidates),
                 [targetLine](const Mutant* m) { return m->getLast().line >= targetLine; });

    if (lineCandidates.empty()) {
      continue;
    }
    mLinesByPath[canonPath]++;
    std::shuffle(lineCandidates.begin(), lineCandidates.end(), rng);
    for (const auto* candidate : lineCandidates) {
      if (selectedSet.insert(*candidate).second) {
        candidates.push_back(*candidate);
        break;
      }
    }
  }

  std::size_t n = candidates.size();
  mCandidateCount = n;
  if (maxMutants == 0 || n <= maxMutants) {
    return candidates;
  }

  // Partial Fisher-Yates: select maxMutants from candidates pool.
  for (std::size_t i = 0; i < maxMutants; ++i) {
    std::uniform_int_distribution<std::size_t> dist(i, n - 1);
    std::size_t j = dist(rng);
    std::swap(candidates[i], candidates[j]);
  }
  return Mutants(candidates.begin(), candidates.begin() + maxMutants);
}

RandomMutantGenerator::SentinelASTVisitor::SentinelASTVisitor(clang::ASTContext* context, Mutants* mutables,
                                                              const std::vector<std::size_t>& targetLines,
                                                              const std::vector<std::string>& selectedOps) :
    mContext(context), mSrcMgr(context->getSourceManager()), mMutants(mutables), mTargetLines(targetLines) {
  std::sort(mTargetLines.begin(), mTargetLines.end());
  auto include = [&selectedOps](const std::string& name) {
    return selectedOps.empty() || std::find(selectedOps.begin(), selectedOps.end(), name) != selectedOps.end();
  };
  if (include("AOR")) mMutationOperators.push_back(std::make_unique<AOR>(context));
  if (include("BOR")) mMutationOperators.push_back(std::make_unique<BOR>(context));
  if (include("ROR")) mMutationOperators.push_back(std::make_unique<ROR>(context));
  if (include("SOR")) mMutationOperators.push_back(std::make_unique<SOR>(context));
  if (include("LCR")) mMutationOperators.push_back(std::make_unique<LCR>(context));
  if (include("SDL")) mMutationOperators.push_back(std::make_unique<SDL>(context));
  if (include("UOI")) mMutationOperators.push_back(std::make_unique<UOI>(context));
}

RandomMutantGenerator::SentinelASTVisitor::~SentinelASTVisitor() = default;

bool RandomMutantGenerator::SentinelASTVisitor::VisitStmt(clang::Stmt* s) {
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

  // Binary search: check if any sorted target line falls in [startLineNum, endLineNum]
  auto lo = std::lower_bound(mTargetLines.begin(), mTargetLines.end(), startLineNum);
  bool containTargetLine = lo != mTargetLines.end() && *lo <= endLineNum;

  if (containTargetLine) {
    for (const auto& m : mMutationOperators) {
      if (m->canMutate(s)) {
        m->populate(s, mMutants);
      }
    }
  }

  return true;
}

RandomMutantGenerator::SentinelASTConsumer::SentinelASTConsumer(const clang::CompilerInstance& ci, Mutants* mutables,
                                                                const std::vector<std::size_t>& targetLines,
                                                                const std::vector<std::string>& selectedOps) :
    mMutants(mutables), mTargetLines(targetLines), mSelectedOps(selectedOps) {
}

void RandomMutantGenerator::SentinelASTConsumer::HandleTranslationUnit(clang::ASTContext& context) {
  SentinelASTVisitor mVisitor(&context, mMutants, mTargetLines, mSelectedOps);
  mVisitor.TraverseDecl(context.getTranslationUnitDecl());
}

RandomMutantGenerator::GenerateMutantAction::GenerateMutantAction(Mutants* mutables,
                                                                  const std::vector<std::size_t>& targetLines,
                                                                  const std::vector<std::string>& selectedOps) :
    mMutants(mutables), mTargetLines(targetLines), mSelectedOps(selectedOps) {
}

std::unique_ptr<clang::ASTConsumer> RandomMutantGenerator::GenerateMutantAction::CreateASTConsumer(
    clang::CompilerInstance& ci, llvm::StringRef inFile) {
  ci.getDiagnostics().setClient(new clang::IgnoringDiagConsumer());
  return std::unique_ptr<clang::ASTConsumer>(new SentinelASTConsumer(ci, mMutants, mTargetLines, mSelectedOps));
}

void RandomMutantGenerator::GenerateMutantAction::ExecuteAction() {
  // AST Traversal.
  clang::ASTFrontendAction::ExecuteAction();
}

std::unique_ptr<clang::tooling::FrontendActionFactory> RandomMutantGenerator::myNewFrontendActionFactory(
    Mutants* mutables, const std::vector<std::size_t>& targetLines, const std::vector<std::string>& selectedOps) {
  class SimpleFrontendActionFactory : public clang::tooling::FrontendActionFactory {
   public:
    explicit SimpleFrontendActionFactory(Mutants* mutables, const std::vector<std::size_t>& targetLines,
                                         const std::vector<std::string>& selectedOps) :
        mMutants(mutables), mTargetLines(targetLines), mSelectedOps(selectedOps) {
    }

#if LLVM_VERSION_MAJOR >= 10
    std::unique_ptr<clang::FrontendAction> create() override {
      return std::make_unique<GenerateMutantAction>(mMutants, mTargetLines, mSelectedOps);
    }
#else
    clang::FrontendAction* create() override {
      return new GenerateMutantAction(mMutants, mTargetLines, mSelectedOps);
    }
#endif

   private:
    Mutants* mMutants;
    const std::vector<std::size_t>& mTargetLines;
    const std::vector<std::string>& mSelectedOps;
  };

  return std::unique_ptr<clang::tooling::FrontendActionFactory>(
      new SimpleFrontendActionFactory(mutables, targetLines, selectedOps));
}

}  // namespace sentinel
