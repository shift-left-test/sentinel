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

  // Pre-compute literal operand info to avoid repeated AST queries in the filter.
  int64_t lhsVal = 0;
  int64_t rhsVal = 0;
  bool lhsIsLit = getIntegerLiteralValue(lhs, &lhsVal);
  bool rhsIsLit = getIntegerLiteralValue(rhs, &rhsVal);

  // RHS is literal 0: x+0 <-> x-0 equivalent, x*0/x%0/x/0 dangerous or trivial.
  if (rhsIsLit && rhsVal == 0) {
    return;
  }

  std::string token{bo->getOpcodeStr()};

  auto filter = [this, lhs, rhs, lhsIsLit, lhsVal, rhsIsLit, rhsVal,
                 &token](const std::string& mutatedToken) -> bool {
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

    // LHS is literal 0: 0*x, 0/x, 0%x all produce 0 — skip among multiplicative ops.
    if (lhsIsLit && lhsVal == 0) {
      if ((token == "*" || token == "/" || token == "%") &&
          (mutatedToken == "*" || mutatedToken == "/" || mutatedToken == "%")) {
        return false;
      }
    }

    // RHS is literal 1: x*1 <-> x/1 equivalent.
    if (rhsIsLit && rhsVal == 1) {
      if ((token == "*" || token == "/") && (mutatedToken == "*" || mutatedToken == "/")) {
        return false;
      }
    }

    return true;
  };

  populateBinaryReplacements(bo, s, mArithmeticOperators, mutables, filter);
}

}  // namespace sentinel
