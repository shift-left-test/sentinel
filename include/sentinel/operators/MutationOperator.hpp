/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_OPERATORS_MUTATIONOPERATOR_HPP_
#define INCLUDE_SENTINEL_OPERATORS_MUTATIONOPERATOR_HPP_

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <functional>
#include <set>
#include <vector>
#include <string>
#include "sentinel/Mutants.hpp"

namespace sentinel {

/**
 * @brief MutationOperator abstract class
 */
class MutationOperator {
 public:
  /**
   * @brief Default constructor
   *
   * @param name of mutation operator
   * @param context Clang ASTContext object
   */
  MutationOperator(const std::string& name, clang::ASTContext* context) :
      mName(name), mContext(context), mSrcMgr(context->getSourceManager()) {
  }

  /**
   * @brief Default destructor
   */
  virtual ~MutationOperator();

  /**
   * @brief Return the name of the mutation operator
   *
   * @return name of mutation operator
   */
  std::string getName() const {
    return mName;
  }

  /**
   * @brief Return True if this mutation operator can be applied to given AST
   *        node. Consider the domain of the mutation operator as well as
   *        the AST node's surrounding context to avoid generating stillborn
   *        mutants.
   *
   * @param s target AST node
   */
  virtual bool canMutate(clang::Stmt* s) = 0;

  /**
   * @brief Create Mutant from given statement.
   *
   * @param s target AST node
   * @param mutables to store newly created mutable
   */
  virtual void populate(clang::Stmt* s, Mutants* mutables) = 0;

  /**
   * @brief Return the code representing given AST node.
   *
   * @param s target AST node.
   * @return code string representing s
   */
  std::string convertStmtToString(const clang::Stmt* s);

  /**
   * @brief Return the parent AST node of given node.
   *
   * @param s target AST node
   * @return parent ast node of s
   */
  const clang::Stmt* getParentStmt(const clang::Stmt* s);

  /**
   * @brief Return type object of the given AST expression node.
   *
   * @param e target AST node
   * @return type object of e
   */
  const clang::Type* getExprType(clang::Expr* e);

  /**
   * @brief Return True if AST node represents dereference reference expression.
   *
   * @param s target AST node
   * @return True/False
   */
  bool isDeclRefExpr(clang::Stmt* s);

  /**
   * @brief Return True if AST node represents pointer dereference expression.
   *
   * @param s target AST node
   * @return True/False
   */
  bool isPointerDereferenceExpr(clang::Stmt* s);

  /**
   * @brief Return the qualified name of the function surrounding this ast node.
   *
   * @param s target AST node
   * @return function qualified name
   */
  std::string getContainingFunctionQualifiedName(clang::Stmt* s);

  /**
   * @brief Return True if the given source code range is valid for
   *        generating Mutant. A source code range is valid if the
   *        start and end locations are
   *        (1) valid locations,
   *        (2) not macro location, and
   *        (3) resides in main target file of ClangTool.
   *
   * @param startLoc start location of source range
   * @param endLoc end location of source range
   * @return True if range is valid
   */
  bool isValidMutantSourceRange(clang::SourceLocation* startLoc, clang::SourceLocation* endLoc);

 protected:
  /**
   * @brief Emit mutants for each replacement token in @p operators that
   *        differs from the current operator token of @p bo.
   *
   * Computes the operator source range from @p bo, validates it, and for
   * every candidate token in @p operators that passes @p filter, appends a
   * new Mutant to @p mutables.
   *
   * @param bo        binary operator AST node (must not be null)
   * @param s         AST statement used to look up the enclosing function name
   * @param operators set of replacement operator tokens
   * @param mutables  output list to append new mutants to
   * @param filter    optional per-token predicate; return false to skip a token
   */
  void populateBinaryReplacements(
      clang::BinaryOperator* bo, clang::Stmt* s,
      const std::set<std::string>& operators, Mutants* mutables,
      const std::function<bool(const std::string&)>& filter = {});

  /**
   * @brief name of mutation operator
   */
  std::string mName;

  /**
   * @brief Clang AST Context object
   */
  clang::ASTContext* mContext;

  /**
   * @brief Object handles loading and caching of source files into memory
   */
  clang::SourceManager& mSrcMgr;
};

/**
 * @brief Create mutation operator instances filtered by selectedOps.
 *        If selectedOps is empty, all operators are created.
 *
 * @param context Clang AST context
 * @param selectedOps list of operator names (e.g. "AOR", "BOR")
 * @return vector of mutation operator instances
 */
std::vector<std::unique_ptr<MutationOperator>> createOperators(
    clang::ASTContext* context, const std::vector<std::string>& selectedOps);

/**
 * @brief Resolve macro expansion and compute the expansion line range
 *        for a given Clang statement.
 *
 * If the statement's begin/end locations are inside a macro, they are
 * resolved to the immediate expansion location.  The resulting source
 * locations are then mapped to expansion line numbers.
 *
 * @param s        target AST statement
 * @param srcMgr   Clang SourceManager
 * @param langOpts Clang LangOptions
 * @param startLineNum receives the expansion start line number
 * @param endLineNum   receives the expansion end line number
 */
void resolveExpansionLineRange(clang::Stmt* s, clang::SourceManager* srcMgr,
                               const clang::LangOptions& langOpts,
                               std::size_t* startLineNum,
                               std::size_t* endLineNum);

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_OPERATORS_MUTATIONOPERATOR_HPP_
