/*
  MIT License

  Copyright (c) 2020 Sung Gon Kim

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef INCLUDE_SENTINEL_UTIL_ASTNODE_HPP_
#define INCLUDE_SENTINEL_UTIL_ASTNODE_HPP_

#include <string>
#include "clang/AST/ASTContext.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Type.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Lexer.h"


namespace sentinel {
namespace util {
namespace astnode {

/**
 * @brief Return the starting Expansion SourceLocation of given SourceLocation.
 *        Expansion SourceLocation (for MACRO) is the location where the MACRO
 *        is used.
 *        The given SourceLocation, which is Spelling SourceLocation is the
 *        location of the target code inside MACRO definition.
 *
 * @param loc Spelling SourceLocation of code expanded from a MACRO usage
 * @param CI compiler instance
 * @return starting Expansion SourceLocation
 */
inline clang::SourceLocation getStartExpansionLocation(
    clang::SourceLocation loc, clang::CompilerInstance& CI) {
  clang::CharSourceRange range = \
      CI.getSourceManager().getImmediateExpansionRange(loc);
  return range.getBegin();
}

/**
 * @brief Return the ending Expansion SourceLocation of given SourceLocation.
 *        Expansion SourceLocation (for MACRO) is the location where the MACRO
 *        is used.
 *        The given SourceLocation, which is Spelling SourceLocation is the
 *        location of the target code inside MACRO definition.
 *
 * @param loc Spelling SourceLocation of code expanded from a MACRO usage
 * @param CI compiler instance
 * @return ending Expansion SourceLocation
 */
inline clang::SourceLocation getEndExpansionLocation(
    clang::SourceLocation loc, clang::CompilerInstance& CI) {
  clang::CharSourceRange range = \
      CI.getSourceManager().getImmediateExpansionRange(loc);
  return clang::Lexer::getLocForEndOfToken(
      range.getEnd(), 0, CI.getSourceManager(), CI.getLangOpts());
}

/**
 * @brief Return the code representing given AST node.
 *
 * @param s target AST node
 * @param CI compiler instance
 * @return code string representing s
 */
inline std::string convertStmtToString(
    clang::Stmt *s, clang::CompilerInstance &CI) {
  clang::SourceLocation walkLoc = s->getBeginLoc();
  clang::SourceLocation endLoc = clang::Lexer::getLocForEndOfToken(
      s->getEndLoc(), 0, CI.getSourceManager(), CI.getLangOpts());
  // clang::SourceLocation endLoc = s->getEndLoc();
  std::string ret = "";

  while (walkLoc != endLoc) {
    ret += *CI.getSourceManager().getCharacterData(walkLoc);
    walkLoc = walkLoc.getLocWithOffset(1);
  }

  return ret;
}

/**
 * @brief Return the parent AST node of given node.
 *
 * @param s target AST node
 * @param CI compiler instance
 * @return parent ast node of s
 */
inline const clang::Stmt* getParentStmt(const clang::Stmt* s,
                                        clang::CompilerInstance& CI) {
  const auto parent = CI.getASTContext().getParents(*s);
  if (parent.empty())
    return nullptr;

  const clang::Stmt* ret = parent[0].get<clang::Stmt>();
  if (!ret)
    return nullptr;

  return ret;
}

/**
 * @brief Return type object of the given AST expression node.
 *
 * @param e target AST node
 * @return type object of e
 */
inline const clang::Type* getExprType(clang::Expr* e) {
  return e->getType().getCanonicalType().getTypePtr();
}

/**
 * @brief Return True if AST node represents dereference reference expression.
 *
 * @param s target AST node
 * @return True/False
 */
inline bool isDeclRefExpr(clang::Stmt *s) {
  if (auto dre = clang::dyn_cast<clang::DeclRefExpr>(s)) {
    // An Enum value is not a declaration reference
    return !clang::isa<clang::EnumConstantDecl>(dre->getDecl());
  }

  return false;
}

/**
 * @brief Return True if AST node represents pointer dereference expression.
 *
 * @param s target AST node
 * @return True/False
 */
inline bool isPointerDereferenceExpr(clang::Stmt *s) {
  if (auto uo = clang::dyn_cast<clang::UnaryOperator>(s)) {
    return (uo->getOpcode() == clang::UO_Deref);
  }

  return false;
}

}  // namespace astnode
}  // namespace util
}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_UTIL_ASTNODE_HPP_

