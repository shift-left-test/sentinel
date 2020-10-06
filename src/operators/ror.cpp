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

#include <clang/AST/Expr.h>
#include <clang/Lex/Lexer.h>
#include <string>
#include "sentinel/operators/ror.hpp"

namespace sentinel {

bool ROR::canMutate(clang::Stmt* s) {
  auto bo = clang::dyn_cast<clang::BinaryOperator>(s);
  if (bo == nullptr) {
    return false;
  }

  return mRelationalOperators.find(bo->getOpcodeStr()) != \
         mRelationalOperators.end();
}

void ROR::populate(clang::Stmt* s, Mutants* mutables) {
  auto bo = clang::dyn_cast<clang::BinaryOperator>(s);

  // create mutables by changing the operator
  std::string token{bo->getOpcodeStr()};
  clang::SourceLocation opStartLoc = bo->getOperatorLoc();
  clang::SourceLocation opEndLoc = mSrcMgr.translateLineCol(
      mSrcMgr.getMainFileID(),
      mSrcMgr.getExpansionLineNumber(opStartLoc),
      mSrcMgr.getExpansionColumnNumber(opStartLoc) + token.length());

  if (isValidMutantSourceRange(&opStartLoc, &opEndLoc)) {
    std::string path = mSrcMgr.getFilename(opStartLoc);
    std::string func = getContainingFunctionQualifiedName(s);

    for (const auto& mutatedToken : mRelationalOperators) {
      if (mutatedToken != token) {
        mutables->emplace_back(
            mName, path, func,
            mSrcMgr.getExpansionLineNumber(opStartLoc),
            mSrcMgr.getExpansionColumnNumber(opStartLoc),
            mSrcMgr.getExpansionLineNumber(opEndLoc),
            mSrcMgr.getExpansionColumnNumber(opEndLoc),
            mutatedToken);
      }
    }
  }

  // create mutables by changing the whole expression to true(1) and false(0)
  clang::SourceLocation stmtStartLoc = bo->getBeginLoc();
  clang::SourceLocation stmtEndLoc = clang::Lexer::getLocForEndOfToken(
      bo->getEndLoc(), 0, mSrcMgr, mContext->getLangOpts());
  if (!isValidMutantSourceRange(&stmtStartLoc, &stmtEndLoc)) {
    return;
  }

  std::string path = mSrcMgr.getFilename(stmtStartLoc);
  std::string func = getContainingFunctionQualifiedName(s);

  mutables->emplace_back(
      mName, path, func,
      mSrcMgr.getExpansionLineNumber(stmtStartLoc),
      mSrcMgr.getExpansionColumnNumber(stmtStartLoc),
      mSrcMgr.getExpansionLineNumber(stmtEndLoc),
      mSrcMgr.getExpansionColumnNumber(stmtEndLoc),
      "1");
  mutables->emplace_back(
      mName, path, func,
      mSrcMgr.getExpansionLineNumber(stmtStartLoc),
      mSrcMgr.getExpansionColumnNumber(stmtStartLoc),
      mSrcMgr.getExpansionLineNumber(stmtEndLoc),
      mSrcMgr.getExpansionColumnNumber(stmtEndLoc),
      "0");
}

}  // namespace sentinel
