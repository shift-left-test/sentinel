/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_OPERATORS_MUTATIONOPERATOR_HPP_
#define INCLUDE_SENTINEL_OPERATORS_MUTATIONOPERATOR_HPP_

#include <clang/AST/ASTContext.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
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
   * @param Context Clang ASTContext object
   */
  MutationOperator(const std::string& name, clang::ASTContext* Context) :
      mName(name), mContext(Context), mSrcMgr(Context->getSourceManager()) {
  }

  /**
   * @brief Default destructor
   */
  virtual ~MutationOperator() {}

  /**
   * @brief Return the name of the mutation operator
   *
   * @return name of mutation operator
   */
  std::string getName() const {
    return mName;
  }

  /**
   * @brief Return True if this mutation operator can be applied to give AST
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
  bool isPointerDereferenceExpr(clang::Stmt *s);

  /**
   * @brief Return the qualified name of the function surround this ast node.
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
  bool isValidMutantSourceRange(clang::SourceLocation *startLoc,
                                 clang::SourceLocation *endLoc);

 protected:
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

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_OPERATORS_MUTATIONOPERATOR_HPP_
