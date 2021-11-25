/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <clang/Lex/Lexer.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <fmt/core.h>
#include <term.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>
#include <map>
#include <random>
#include <vector>
#include "sentinel/Logger.hpp"
#include "sentinel/Mutants.hpp"
#include "sentinel/ncstream/term.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/RandomMutantGenerator.hpp"
#include "sentinel/exceptions/IOException.hpp"


namespace sentinel {

static const char * cRandomGeneratorLoggerName = "RandomMutantGenerator";

Mutants RandomMutantGenerator::populate(const SourceLines& sourceLines,
                                        std::size_t maxMutants) {
  return populate(sourceLines, maxMutants, std::random_device {}());
}

Mutants RandomMutantGenerator::populate(const SourceLines& sourceLines,
                                        std::size_t maxMutants,
                                        unsigned randomSeed) {
  Mutants mutables;

  std::string errorMsg;
  std::unique_ptr<clang::tooling::CompilationDatabase> compileDb = \
      clang::tooling::CompilationDatabase::loadFromDirectory(mDbPath,
                                                             errorMsg);

  if (compileDb == nullptr) {
    throw IOException(EINVAL, errorMsg);
  }

  // convert sourceLines to map from filename to list of target source lines
  std::map<std::string, std::vector<std::size_t>> targetLines;

  for (const auto& sourceLine : sourceLines) {
    std::string filename = sourceLine.getPath();
    auto it = targetLines.find(filename);

    if (it == targetLines.end()) {
      targetLines[filename] = std::vector<std::size_t>();
    }

    targetLines[filename].push_back(sourceLine.getLineNumber());
  }

  auto logger = Logger::getLogger(cRandomGeneratorLoggerName);
  logger->info(fmt::format("random seed: {}", randomSeed));

  for (const auto& file : targetLines) {
    logger->info(fmt::format("Checking for mutants in {}", file.first));
    clang::tooling::ClangTool tool(*compileDb, file.first);
    tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster(
        "-ferror-limit=0"));

    // Save the current TERMINAL object (created by ncurses::initscr)
    // and set the current TERMINAL pointer to nullptr (might be error-prone)
    // before running ClangTool::run.
    //
    // LLVM contains a bug which delete the TERMINAL object, causing
    // segmentation fault when used together with ncurses.
    // Refer to the following link for more details:
    //  - http://www.brendangregg.com/blog/2016-08-09/gdb-example-ncurses.html
    //
    // LLVM code (release/10.x) which delete TERMINAL object is in
    // llvm-project/llvm/lib/Support/Unix/Process.inc::terminalHasColors::360
    term = set_curterm(nullptr);
    tool.run(myNewFrontendActionFactory(&mutables, file.second).get());
    set_curterm(term);
    term = nullptr;
  }

  if (mutables.size() <= maxMutants) {
    return Mutants(mutables.begin(), mutables.end());
  }

  mutables.shuffle(randomSeed);
  return Mutants(mutables.begin(), mutables.begin() + maxMutants);
}

RandomMutantGenerator::SentinelASTVisitor::SentinelASTVisitor(
    clang::ASTContext* Context, Mutants* mutables,
    const std::vector<std::size_t>& targetLines) :
    mContext(Context), mSrcMgr(Context->getSourceManager()),
    mMutants(mutables), mTargetLines(targetLines) {
  mMutationOperators.push_back(new AOR(Context));
  mMutationOperators.push_back(new BOR(Context));
  mMutationOperators.push_back(new ROR(Context));
  mMutationOperators.push_back(new SOR(Context));
  mMutationOperators.push_back(new LCR(Context));
  mMutationOperators.push_back(new SDL(Context));
  mMutationOperators.push_back(new UOI(Context));
}

RandomMutantGenerator::SentinelASTVisitor::~SentinelASTVisitor() {
  for (auto op : mMutationOperators) {
    delete op;
  }

  mMutationOperators.clear();
}

bool RandomMutantGenerator::SentinelASTVisitor::VisitStmt(clang::Stmt* s) {
  clang::SourceLocation startLoc = s->getBeginLoc();
  clang::SourceLocation endLoc = s->getEndLoc();

  // Check if this Stmt node represents code on target lines.
  if (startLoc.isMacroID()) {
    clang::CharSourceRange range = mContext->getSourceManager()
        .getImmediateExpansionRange(startLoc);
    startLoc = range.getBegin();
  }

  if (endLoc.isMacroID()) {
    clang::CharSourceRange range = mContext->getSourceManager()
        .getImmediateExpansionRange(endLoc);
    endLoc = clang::Lexer::getLocForEndOfToken(
        range.getEnd(), 0, mContext->getSourceManager(),
        mContext->getLangOpts());
  }

  std::size_t startLineNum = mSrcMgr.getExpansionLineNumber(startLoc);
  std::size_t endLineNum = mSrcMgr.getExpansionLineNumber(endLoc);
  bool containTargetLine = std::any_of(mTargetLines.begin(), mTargetLines.end(),
      [startLineNum, endLineNum](std::size_t lineNum)
      { return lineNum >= startLineNum && lineNum <= endLineNum; } );

  if (containTargetLine) {
    for (auto m : mMutationOperators) {
      if (m->canMutate(s)) {
        m->populate(s, mMutants);
      }
    }
  }

  return true;
}

void RandomMutantGenerator::GenerateMutantAction::ExecuteAction() {
  // AST Traversal.
  clang::ASTFrontendAction::ExecuteAction();
}

std::unique_ptr<clang::tooling::FrontendActionFactory>
RandomMutantGenerator::myNewFrontendActionFactory(
    Mutants* mutables, const std::vector<std::size_t>& targetLines) {
  class SimpleFrontendActionFactory :
          public clang::tooling::FrontendActionFactory {
   public:
    explicit SimpleFrontendActionFactory(
        Mutants* mutables,
        const std::vector<std::size_t>& targetLines) :
        mMutants(mutables), mTargetLines(targetLines) {
    }

#if LLVM_VERSION_MAJOR >= 10
    std::unique_ptr<clang::FrontendAction> create() override {
      return std::make_unique<GenerateMutantAction>(mMutants, mTargetLines);
    }
#else
    clang::FrontendAction *create() override {
      return new GenerateMutantAction(mMutants, mTargetLines);
    }
#endif

   private:
    Mutants* mMutants;
    const std::vector<std::size_t>& mTargetLines;
  };

  return std::unique_ptr<clang::tooling::FrontendActionFactory>(
      new SimpleFrontendActionFactory(mutables, targetLines));
}

}  // namespace sentinel
