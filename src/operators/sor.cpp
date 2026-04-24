/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <clang/AST/Expr.h>
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
  const clang::Expr* lhs = bo->getLHS()->IgnoreImpCasts();
  const clang::Expr* rhs = bo->getRHS()->IgnoreImpCasts();

  // Skip all mutations when either operand is literal 0 (equivalent mutants).
  int64_t val = 0;
  if ((getIntegerLiteralValue(lhs, &val) && val == 0) ||
      (getIntegerLiteralValue(rhs, &val) && val == 0)) {
    return;
  }

  populateBinaryReplacements(bo, s, mShiftOperators, mutables);
}

}  // namespace sentinel
