/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/Expr.h>
#include <clang/Lex/Lexer.h>
#include <string>
#include "sentinel/operators/aor.hpp"

namespace sentinel {

bool AOR::canMutate(clang::Stmt* s) {
  auto bo = clang::dyn_cast<clang::BinaryOperator>(s);
  if (bo == nullptr) {
    return false;
  }

  return mArithmeticOperators.find(std::string(bo->getOpcodeStr())) != \
         mArithmeticOperators.end();
}

void AOR::populate(clang::Stmt* s, Mutants* mutables) {
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

    for (const auto& mutatedToken : mArithmeticOperators) {
      if (mutatedToken == token) {
        continue;
      }

      clang::Expr *lhs = bo->getLHS()->IgnoreImpCasts();
      clang::Expr *rhs = bo->getRHS()->IgnoreImpCasts();

      // 2 pointers can only minus each other, so no mutables are generated.
      if ((getExprType(lhs)->isPointerType() ||
           getExprType(lhs)->isArrayType()) &&
          (getExprType(rhs)->isPointerType() ||
           getExprType(rhs)->isArrayType())) {
        continue;
      }

      // modulo operator only takes integral operands.
      if (mutatedToken == "%" &&
          (!getExprType(lhs)->isIntegralType(*mContext) ||
           !getExprType(rhs)->isIntegralType(*mContext))) {
        continue;
      }

      // multiplicative operator only takes non-pointer operands.
      if ((mutatedToken == "*" || mutatedToken == "/") &&
          (getExprType(lhs)->isPointerType() ||
           getExprType(rhs)->isPointerType() ||
           getExprType(lhs)->isArrayType() ||
           getExprType(rhs)->isArrayType())) {
        continue;
      }

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

}  // namespace sentinel
