/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <clang/AST/Expr.h>
#include <string>
#include "sentinel/operators/bor.hpp"

namespace sentinel {

bool BOR::canMutate(clang::Stmt* s) {
  auto bo = clang::dyn_cast<clang::BinaryOperator>(s);
  if (bo == nullptr) {
    return false;
  }

  return mBitwiseOperators.find(std::string(bo->getOpcodeStr())) != mBitwiseOperators.end();
}

void BOR::populate(clang::Stmt* s, Mutants* mutables) {
  auto bo = clang::dyn_cast<clang::BinaryOperator>(s);
  populateBinaryReplacements(bo, s, mBitwiseOperators, mutables);
}

}  // namespace sentinel
