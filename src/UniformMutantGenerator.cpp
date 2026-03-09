/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <clang/Lex/Lexer.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <fmt/core.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <future>
#include <iostream>
#include <map>
#include <thread>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>
#include "sentinel/Mutants.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/UniformMutantGenerator.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace fs = std::filesystem;

namespace sentinel {

static const char* cUniformGeneratorLoggerName = "UniformMutantGenerator";

UniformMutantGenerator::UniformMutantGenerator(const std::string& path) : mDbPath(path) {
}

Mutants UniformMutantGenerator::populate(const SourceLines& sourceLines, std::size_t maxMutants) {
  return populate(sourceLines, maxMutants, std::random_device {}());
}

Mutants UniformMutantGenerator::populate(const SourceLines& sourceLines, std::size_t maxMutants, unsigned randomSeed) {
  Mutants mutables;

  std::string errorMsg;
  std::unique_ptr<clang::tooling::CompilationDatabase> compileDb =
      clang::tooling::CompilationDatabase::loadFromDirectory(mDbPath, errorMsg);

  if (compileDb == nullptr) {
    throw IOException(EINVAL, errorMsg);
  }

  // convert sourceLines to map from filename to list of target source lines
  std::map<std::string, std::vector<std::size_t>> targetLines;

  for (const auto& sourceLine : sourceLines) {
    std::string filename = sourceLine.getPath();
    targetLines[filename].push_back(sourceLine.getLineNumber());
  }

  auto logger = Logger::getLogger(cUniformGeneratorLoggerName);
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
      logger->verbose(fmt::format("Checking for mutants in {}", fileIt->first));
      futures.push_back(
          std::async(std::launch::async, [db, filename = fileIt->first, lines = fileIt->second, this]() {
            Mutants localMutables;
            clang::tooling::ClangTool tool(*db, filename);
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

  // Pre-group mutants by canonical file path, sorted by first.line for binary search.
  // Mutant::mPath is already canonical (set in Mutant constructor).
  std::map<std::string, std::vector<const Mutant*>> mutantsByFile;
  for (const auto& m : mutables) {
    mutantsByFile[m.getPath().string()].push_back(&m);
  }
  for (auto& entry : mutantsByFile) {
    std::sort(entry.second.begin(), entry.second.end(), [](const Mutant* a, const Mutant* b) {
      return a->getFirst().line < b->getFirst().line;
    });
  }

  // Randomly select one Mutant on each target line.
  // Cache canonical path per unique source path to avoid repeated fs::canonical() calls.
  std::map<std::string, std::string> pathCache;
  std::set<Mutant> selectedSet;
  Mutants temp_storage;
  std::mt19937 rng(randomSeed);

  for (const auto& line : sourceLines) {
    std::string rawPath = line.getPath().string();
    auto emplaceResult = pathCache.emplace(rawPath, std::string{});
    if (emplaceResult.second) {
      emplaceResult.first->second = fs::canonical(rawPath).string();
    }
    const std::string& canonPath = emplaceResult.first->second;
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

    if (candidates.empty()) {
      continue;
    }

    std::shuffle(candidates.begin(), candidates.end(), rng);

    // find first candidate not already in selectedSet
    for (const auto* candidate : candidates) {
      if (selectedSet.insert(*candidate).second) {
        temp_storage.push_back(*candidate);
        break;
      }
    }

    if (temp_storage.size() == maxMutants) {
      break;
    }
  }

  mCandidateCount = mutables.size();
  return temp_storage;
}

UniformMutantGenerator::SentinelASTVisitor::SentinelASTVisitor(clang::ASTContext* Context, Mutants* mutables,
                                                               const std::vector<std::size_t>& targetLines,
                                                               const std::vector<std::string>& selectedOps) :
    mContext(Context), mSrcMgr(Context->getSourceManager()), mMutants(mutables), mTargetLines(targetLines) {
  std::sort(mTargetLines.begin(), mTargetLines.end());
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

UniformMutantGenerator::SentinelASTVisitor::~SentinelASTVisitor() = default;

bool UniformMutantGenerator::SentinelASTVisitor::VisitStmt(clang::Stmt* s) {
  clang::SourceLocation startLoc = s->getBeginLoc();
  clang::SourceLocation endLoc = s->getEndLoc();

  // Check if this Stmt node represents code on target lines.
  if (startLoc.isMacroID()) {
    clang::CharSourceRange range = mSrcMgr.getImmediateExpansionRange(startLoc);
    startLoc = range.getBegin();
  }

  if (endLoc.isMacroID()) {
    clang::CharSourceRange range = mSrcMgr.getImmediateExpansionRange(endLoc);
    endLoc = clang::Lexer::getLocForEndOfToken(range.getEnd(), 0, mSrcMgr, mContext->getLangOpts());
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

UniformMutantGenerator::SentinelASTConsumer::SentinelASTConsumer(const clang::CompilerInstance& CI,
                                                                 Mutants* mutables,
                                                                 const std::vector<std::size_t>& targetLines,
                                                                 const std::vector<std::string>& selectedOps) :
    mMutants(mutables), mTargetLines(targetLines), mSelectedOps(selectedOps) {
}

void UniformMutantGenerator::SentinelASTConsumer::HandleTranslationUnit(clang::ASTContext& Context) {
  SentinelASTVisitor mVisitor(&Context, mMutants, mTargetLines, mSelectedOps);
  mVisitor.TraverseDecl(Context.getTranslationUnitDecl());
}

UniformMutantGenerator::GenerateMutantAction::GenerateMutantAction(Mutants* mutables,
                                                                   const std::vector<std::size_t>& targetLines,
                                                                   const std::vector<std::string>& selectedOps) :
    mMutants(mutables), mTargetLines(targetLines), mSelectedOps(selectedOps) {
}

std::unique_ptr<clang::ASTConsumer> UniformMutantGenerator::GenerateMutantAction::CreateASTConsumer(
    clang::CompilerInstance& CI, llvm::StringRef InFile) {
  CI.getDiagnostics().setClient(new clang::IgnoringDiagConsumer());
  return std::unique_ptr<clang::ASTConsumer>(new SentinelASTConsumer(CI, mMutants, mTargetLines, mSelectedOps));
}

void UniformMutantGenerator::GenerateMutantAction::ExecuteAction() {
  // AST Traversal.
  clang::ASTFrontendAction::ExecuteAction();
}

std::unique_ptr<clang::tooling::FrontendActionFactory> UniformMutantGenerator::myNewFrontendActionFactory(
    Mutants* mutables, const std::vector<std::size_t>& targetLines, const std::vector<std::string>& selectedOps) {
  class SimpleFrontendActionFactory : public clang::tooling::FrontendActionFactory {
   public:
    explicit SimpleFrontendActionFactory(Mutants* mutables, const std::vector<std::size_t>& targetLines,
                                         const std::vector<std::string>& selectedOps) :
        mMutants(mutables), mTargetLines(targetLines), mSelectedOps(selectedOps) {}

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
