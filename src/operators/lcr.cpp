/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <clang/AST/Expr.h>
#include <clang/Lex/Lexer.h>
#include <string>
#include "sentinel/operators/lcr.hpp"

namespace sentinel {

bool LCR::canMutate(clang::Stmt* s) {
  auto bo = clang::dyn_cast<clang::BinaryOperator>(s);
  if (bo == nullptr) {
    return false;
  }

  return mLogicalOperators.find(std::string(bo->getOpcodeStr())) != mLogicalOperators.end();
}

void LCR::populate(clang::Stmt* s, Mutants* mutables) {
  auto bo = clang::cast<clang::BinaryOperator>(s);

  // create mutables by changing the operator
  populateBinaryReplacements(bo, s, mLogicalOperators, mutables);

  // Skip true(1)/false(0) replacement in loop conditions to prevent infinite loops.
  if (isLoopCondition(s)) {
    return;
  }

  // create mutables by changing the whole expression to true(1) and false(0)
  clang::SourceLocation stmtStartLoc = bo->getBeginLoc();
  clang::SourceLocation stmtEndLoc =
      clang::Lexer::getLocForEndOfToken(bo->getEndLoc(), 0, mSrcMgr, mContext->getLangOpts());

  if (!isValidMutantSourceRange(&stmtStartLoc, &stmtEndLoc)) {
    return;
  }

  std::string path{mSrcMgr.getFilename(stmtStartLoc)};
  std::string func = getContainingFunctionQualifiedName(s);
  emitMutant(mutables, path, func, stmtStartLoc, stmtEndLoc, "1");
  emitMutant(mutables, path, func, stmtStartLoc, stmtEndLoc, "0");
}

}  // namespace sentinel
