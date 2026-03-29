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
  populateBinaryReplacements(bo, s, mShiftOperators, mutables);
}

}  // namespace sentinel
