/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <clang/AST/Expr.h>
#include <clang/Lex/Lexer.h>
#include <string>
#include "sentinel/operators/sor.hpp"

namespace sentinel {

bool SOR::canMutate(clang::Stmt* s) {
  auto bo = clang::dyn_cast<clang::BinaryOperator>(s);
  if (bo == nullptr) {
    return false;
  }

  return mShiftOperators.find(std::string(bo->getOpcodeStr())) != mShiftOperators.end();
}

void SOR::populate(clang::Stmt* s, Mutants* mutables) {
  auto bo = clang::dyn_cast<clang::BinaryOperator>(s);
  std::string token{bo->getOpcodeStr()};
  clang::SourceLocation opStartLoc = bo->getOperatorLoc();
  clang::SourceLocation opEndLoc = mSrcMgr.translateLineCol(
      mSrcMgr.getMainFileID(),
      mSrcMgr.getExpansionLineNumber(opStartLoc),
      mSrcMgr.getExpansionColumnNumber(opStartLoc) + token.length());

  if (isValidMutantSourceRange(&opStartLoc, &opEndLoc)) {
    std::string path{mSrcMgr.getFilename(opStartLoc)};
    std::string func = getContainingFunctionQualifiedName(s);

    for (const auto& mutatedToken : mShiftOperators) {
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
}

}  // namespace sentinel
