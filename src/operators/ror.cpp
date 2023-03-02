/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
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

  return mRelationalOperators.find(std::string(bo->getOpcodeStr())) != \
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
    std::string path{mSrcMgr.getFilename(opStartLoc)};
    std::string func = getContainingFunctionQualifiedName(s);
    bool operandIsNull =
        getExprType(bo->getLHS()->IgnoreImpCasts())->isNullPtrType() ||
        getExprType(bo->getRHS()->IgnoreImpCasts())->isNullPtrType();

    for (const auto& mutatedToken : mRelationalOperators) {
      if (operandIsNull && mutatedToken != "==" && mutatedToken != "!=") {
        continue;
      }

      // Excluding tokens that generate fp in corner cases
      auto contains = [](const std::vector<std::string>& haystack,
                         const std::string& needle) {
        return std::find(haystack.begin(), haystack.end(), needle) !=
               haystack.end();
      };
      if (mIgnored.count(token) != 0 &&
          contains(mIgnored.at(token), mutatedToken)) {
        continue;
      }

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

  std::string path{mSrcMgr.getFilename(stmtStartLoc)};
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
