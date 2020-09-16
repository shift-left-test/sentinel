/*
  MIT License

  Copyright (c) 2020 Sung Gon Kim

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

#include <algorithm>
#include <iostream>
#include <map>
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"
#include "sentinel/Mutables.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/UniformMutableGenerator.hpp"
#include "sentinel/exceptions/IOException.hpp"


namespace sentinel {

Mutables UniformMutableGenerator::populate(const std::string& outPath,
                                           const SourceLines& sourceLines) {
  Mutables mutables{outPath};

  std::string errorMsg;
  std::unique_ptr<clang::tooling::CompilationDatabase> compileDb = \
      clang::tooling::CompilationDatabase::loadFromDirectory(mDbPath,
                                                             errorMsg);

  if (compileDb == nullptr) {
    // TODO(Loc Phan): make a new exception class
    throw IOException(EINVAL, errorMsg);
  }

  // convert sourceLines to map from filename to list of target source lines
  std::map<std::string, std::vector<int>> targetLines;

  for (const auto& sourceLine : sourceLines) {
    std::string filename = sourceLine.getPath();
    auto it = targetLines.find(filename);

    if (it == targetLines.end()) {
      targetLines[filename] = std::vector<int>();
    }

    targetLines[filename].push_back(sourceLine.getLineNumber());
  }

  for (const auto& file : targetLines) {
    clang::tooling::ClangTool tool(*compileDb, file.first);
    tool.run(myNewFrontendActionFactory(&mutables, file.second).get());
  }

  // clang::tooling::ClangTool tool(*compileDb, file);
  // tool.run(myNewFrontendActionFactory(&mutables).get());

  return mutables;
}

UniformMutableGenerator::SentinelASTVisitor::SentinelASTVisitor(
    const clang::CompilerInstance& CI, Mutables* mutables,
    const std::vector<int>& targetLines)
    : mCI(CI), mSrcMgr(CI.getSourceManager()),
      mMutables(mutables), mTargetLines(targetLines) {
  mMutationOperators.push_back(new AOR(mCI));
  mMutationOperators.push_back(new BOR(mCI));
  mMutationOperators.push_back(new ROR(mCI));
  mMutationOperators.push_back(new SOR(mCI));
  mMutationOperators.push_back(new LCR(mCI));
  mMutationOperators.push_back(new SDL(mCI));
  mMutationOperators.push_back(new UOI(mCI));
}

bool UniformMutableGenerator::SentinelASTVisitor::VisitStmt(clang::Stmt* s) {
  clang::SourceLocation startLoc = s->getBeginLoc();
  clang::SourceLocation endLoc = s->getEndLoc();

  // Check if this Stmt node represents code on target lines.
  if (startLoc.isMacroID()) {
    clang::CharSourceRange range = mSrcMgr.getImmediateExpansionRange(startLoc);
    startLoc = range.getBegin();
  }

  if (endLoc.isMacroID()) {
    clang::CharSourceRange range = mSrcMgr.getImmediateExpansionRange(endLoc);
    endLoc = clang::Lexer::getLocForEndOfToken(
        range.getEnd(), 0, mSrcMgr, mCI.getLangOpts());
  }

  int startLineNum = mSrcMgr.getExpansionLineNumber(startLoc);
  int endLineNum = mSrcMgr.getExpansionLineNumber(endLoc);
  bool containTargetLine = false;
  for (const auto& lineNum : mTargetLines) {
    if (lineNum >= startLineNum && lineNum <= endLineNum) {
      containTargetLine = true;
      break;
    }
  }

  if (containTargetLine) {
    for (auto m : mMutationOperators) {
      if (m->canMutate(s)) {
        m->populate(s, mMutables);
      }
    }
  }

  return true;
}

void UniformMutableGenerator::GenerateMutantAction::ExecuteAction() {
  // Insert code for pre-traversal preparation.

  // AST Traversal.
  clang::ASTFrontendAction::ExecuteAction();

  // Insert code for post-traversal wrapup.
}

std::unique_ptr<clang::tooling::FrontendActionFactory>
UniformMutableGenerator::myNewFrontendActionFactory(
    Mutables* mutables, const std::vector<int>& targetLines) {
  class SimpleFrontendActionFactory :
      public clang::tooling::FrontendActionFactory {
   public:
    explicit SimpleFrontendActionFactory(Mutables* mutables,
                                         const std::vector<int>& targetLines)
        : mMutables(mutables), mTargetLines(targetLines) {
    }

    std::unique_ptr<clang::FrontendAction> create() override {
      return std::make_unique<GenerateMutantAction>(mMutables, mTargetLines);
    }

   private:
    Mutables* mMutables;
    const std::vector<int>& mTargetLines;
  };

  return std::unique_ptr<clang::tooling::FrontendActionFactory>(
      new SimpleFrontendActionFactory(mutables, targetLines));
}

}  // namespace sentinel
