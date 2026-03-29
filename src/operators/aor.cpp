/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/Expr.h>
#include <string>
#include "sentinel/operators/aor.hpp"

namespace sentinel {

bool AOR::canMutate(clang::Stmt* s) {
  auto bo = clang::dyn_cast<clang::BinaryOperator>(s);
  if (bo == nullptr) {
    return false;
  }

  return mArithmeticOperators.find(std::string(bo->getOpcodeStr())) != mArithmeticOperators.end();
}

void AOR::populate(clang::Stmt* s, Mutants* mutables) {
  auto bo = clang::dyn_cast<clang::BinaryOperator>(s);
  clang::Expr* lhs = bo->getLHS()->IgnoreImpCasts();
  clang::Expr* rhs = bo->getRHS()->IgnoreImpCasts();

  auto filter = [this, lhs, rhs](const std::string& mutatedToken) -> bool {
    // 2 pointers can only minus each other, so no mutables are generated.
    if ((getExprType(lhs)->isPointerType() || getExprType(lhs)->isArrayType()) &&
        (getExprType(rhs)->isPointerType() || getExprType(rhs)->isArrayType())) {
      return false;
    }

    // modulo operator only takes integral operands.
    if (mutatedToken == "%" &&
        (!getExprType(lhs)->isIntegralType(*mContext) || !getExprType(rhs)->isIntegralType(*mContext))) {
      return false;
    }

    // multiplicative operator only takes non-pointer operands.
    if ((mutatedToken == "*" || mutatedToken == "/") &&
        (getExprType(lhs)->isPointerType() || getExprType(rhs)->isPointerType() ||
         getExprType(lhs)->isArrayType() || getExprType(rhs)->isArrayType())) {
      return false;
    }

    return true;
  };

  populateBinaryReplacements(bo, s, mArithmeticOperators, mutables, filter);
}

}  // namespace sentinel
