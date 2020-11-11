/*
  MIT License

  Copyright (c) 2020 Loc Duy Phan

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <clang/Lex/Lexer.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>
#include <map>
#include <random>
#include <vector>
#include "sentinel/Mutants.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/RandomMutantGenerator.hpp"
#include "sentinel/exceptions/IOException.hpp"


namespace sentinel {

Mutants RandomMutantGenerator::populate(const SourceLines& sourceLines,
                                         std::size_t maxMutants) {
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

  for (const auto& file : targetLines) {
    clang::tooling::ClangTool tool(*compileDb, file.first);
    tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster(
        "-ferror-limit=0"));
    tool.run(myNewFrontendActionFactory(&mutables, file.second).get());
  }

  if (mutables.size() <= maxMutants) {
    return Mutants(mutables.begin(), mutables.end());
  }

  mutables.shuffle();
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
