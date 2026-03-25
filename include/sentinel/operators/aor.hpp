/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_OPERATORS_AOR_HPP_
#define INCLUDE_SENTINEL_OPERATORS_AOR_HPP_

#include <set>
#include <string>
#include "sentinel/operators/MutationOperator.hpp"

namespace sentinel {

/**
 * @brief Arithmetic Operator Replacement class
 */
class AOR : public MutationOperator {
 public:
  /**
   * @brief Default constructor
   *
   * @param context Clang ASTContext object
   */
  explicit AOR(clang::ASTContext* context) : MutationOperator("AOR", context) {
  }

  /**
   * @brief Return True if this mutation operator can be applied to give AST
   *        node. Consider the domain of the mutation operator as well as
   *        the AST node's surrounding context to avoid generating stillborn
   *        mutants.
   *
   * @param s target AST node
   */
  bool canMutate(clang::Stmt* s) override;

  /**
   * @brief Create Mutant from given statement.
   *
   * @param s target AST node
   * @param mutables to store newly created mutable
   */
  void populate(clang::Stmt* s, Mutants* mutables) override;

 private:
  std::set<std::string> mArithmeticOperators = {"+", "-", "*", "/", "%"};
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_OPERATORS_AOR_HPP_
