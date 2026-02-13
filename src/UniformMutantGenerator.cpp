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
#include <memory>
#include <random>
#include <string>
#include <vector>
#include "sentinel/Mutants.hpp"
#include "sentinel/ncstream/term.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/UniformMutantGenerator.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace sentinel {

static const char * cUniformGeneratorLoggerName = "UniformMutantGenerator";

UniformMutantGenerator::UniformMutantGenerator(const std::string& path) : mDbPath(path) {
}

Mutants UniformMutantGenerator::populate(const SourceLines& sourceLines, std::size_t maxMutants) {
  return populate(sourceLines, maxMutants, std::random_device {}());
}

Mutants UniformMutantGenerator::populate(const SourceLines& sourceLines, std::size_t maxMutants,
                                         unsigned randomSeed) {
  Mutants mutables;

  std::string errorMsg;
  std::unique_ptr<clang::tooling::CompilationDatabase> compileDb = \
      clang::tooling::CompilationDatabase::loadFromDirectory(mDbPath, errorMsg);

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

  auto logger = Logger::getLogger(cUniformGeneratorLoggerName);
  logger->info(fmt::format("random seed: {}", randomSeed));

  for (const auto& file : targetLines) {
    logger->info(fmt::format("Checking for mutants in {}", file.first));
    clang::tooling::ClangTool tool(*compileDb, file.first);
    tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster("-ferror-limit=0"));

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

  // Randomly select one Mutant on each target line
  Mutants temp_storage;
  for (const auto& line : sourceLines) {
    std::vector<Mutant> temp;
    auto pred = [&](const auto& m) {
      return std::experimental::filesystem::equivalent(
          m.getPath(), line.getPath()) &&
          m.getFirst().line <= line.getLineNumber() &&
          m.getLast().line >= line.getLineNumber();
    };
    std::copy_if(mutables.begin(), mutables.end(), std::back_inserter(temp), pred);

    if (temp.empty()) {
      continue;
    }

    std::shuffle(std::begin(temp), std::end(temp), std::mt19937(randomSeed));
    // find first element of temp that is not in temp_storage
    auto it = std::find_if(temp.begin(), temp.end(),
        [&](const Mutant& a) {
           return std::find(temp_storage.begin(), temp_storage.end(), a) == temp_storage.end();
        });
    if (it != temp.end()) {
      temp_storage.push_back(*it);
    }

    if (temp_storage.size() == maxMutants) {
      break;
    }
  }

  return Mutants(temp_storage.begin(), temp_storage.end());
}

UniformMutantGenerator::SentinelASTVisitor::SentinelASTVisitor(clang::ASTContext* Context, Mutants* mutables,
                                                               const std::vector<std::size_t>& targetLines) :
    mContext(Context), mSrcMgr(Context->getSourceManager()), mMutants(mutables), mTargetLines(targetLines) {
  mMutationOperators.push_back(new AOR(Context));
  mMutationOperators.push_back(new BOR(Context));
  mMutationOperators.push_back(new ROR(Context));
  mMutationOperators.push_back(new SOR(Context));
  mMutationOperators.push_back(new LCR(Context));
  mMutationOperators.push_back(new SDL(Context));
  mMutationOperators.push_back(new UOI(Context));
}

UniformMutantGenerator::SentinelASTVisitor::~SentinelASTVisitor() {
  for (auto op : mMutationOperators) {
    delete op;
  }

  mMutationOperators.clear();
}

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
  bool containTargetLine = std::any_of(mTargetLines.begin(), mTargetLines.end(),
      [startLineNum, endLineNum] (std::size_t lineNum) {
        return lineNum >= startLineNum && lineNum <= endLineNum;
      });

  if (containTargetLine) {
    for (auto m : mMutationOperators) {
      if (m->canMutate(s)) {
        m->populate(s, mMutants);
      }
    }
  }

  return true;
}

UniformMutantGenerator::SentinelASTConsumer::SentinelASTConsumer(const clang::CompilerInstance& CI,
                                                                 Mutants* mutables,
                                                                 const std::vector<std::size_t>& targetLines) :
    mMutants(mutables), mTargetLines(targetLines) {
}

void UniformMutantGenerator::SentinelASTConsumer::HandleTranslationUnit(clang::ASTContext &Context) {
  SentinelASTVisitor mVisitor(&Context, mMutants, mTargetLines);
  mVisitor.TraverseDecl(Context.getTranslationUnitDecl());
}

UniformMutantGenerator::GenerateMutantAction::GenerateMutantAction(Mutants* mutables,
                                                                   const std::vector<std::size_t>& targetLines) :
    mMutants(mutables), mTargetLines(targetLines) {
}

std::unique_ptr<clang::ASTConsumer> UniformMutantGenerator::GenerateMutantAction::CreateASTConsumer(
    clang::CompilerInstance& CI, llvm::StringRef InFile) {
  CI.getDiagnostics().setClient(new clang::IgnoringDiagConsumer());
  return std::unique_ptr<clang::ASTConsumer>(new SentinelASTConsumer(CI, mMutants, mTargetLines));
}

void UniformMutantGenerator::GenerateMutantAction::ExecuteAction() {
  // AST Traversal.
  clang::ASTFrontendAction::ExecuteAction();
}

std::unique_ptr<clang::tooling::FrontendActionFactory>
UniformMutantGenerator::myNewFrontendActionFactory(Mutants* mutables,
                                                   const std::vector<std::size_t>& targetLines) {
  class SimpleFrontendActionFactory :
          public clang::tooling::FrontendActionFactory {
   public:
    explicit SimpleFrontendActionFactory(Mutants* mutables, const std::vector<std::size_t>& targetLines) :
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
