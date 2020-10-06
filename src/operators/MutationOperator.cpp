/*
  MIT License

  Copyright (c) 2020 Loc Duy Phan

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

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Lex/Lexer.h>
#include <string>
#include "sentinel/operators/MutationOperator.hpp"


namespace sentinel {

std::string MutationOperator::convertStmtToString(clang::Stmt* s) {
  clang::SourceLocation walkLoc = s->getBeginLoc();
  clang::SourceLocation endLoc = clang::Lexer::getLocForEndOfToken(
      s->getEndLoc(), 0, mContext->getSourceManager(),
      mContext->getLangOpts());
  std::string ret;

  while (walkLoc != endLoc) {
    ret += *mContext->getSourceManager().getCharacterData(walkLoc);
    walkLoc = walkLoc.getLocWithOffset(1);
  }

  return ret;
}

const clang::Stmt* MutationOperator::getParentStmt(const clang::Stmt* s) {
  const auto parent = mContext->getParents(*s);
  if (parent.empty()) {
    return nullptr;
  }

  const auto ret = parent[0].get<clang::Stmt>();
  if (ret == nullptr) {
    return nullptr;
  }

  return ret;
}

const clang::Type* MutationOperator::getExprType(clang::Expr* e) {
  return e->getType().getCanonicalType().getTypePtr();
}

bool MutationOperator::isDeclRefExpr(clang::Stmt* s) {
  if (auto dre = clang::dyn_cast<clang::DeclRefExpr>(s)) {
    // An Enum value is not a declaration reference
    return !clang::isa<clang::EnumConstantDecl>(dre->getDecl());
  }

  return false;
}

bool MutationOperator::isPointerDereferenceExpr(clang::Stmt *s) {
  if (auto uo = clang::dyn_cast<clang::UnaryOperator>(s)) {
    return (uo->getOpcode() == clang::UO_Deref);
  }

  return false;
}

std::string MutationOperator::getContainingFunctionQualifiedName(
    clang::Stmt* s) {
  const clang::Stmt* stmt = s;
  const clang::Decl* decl = nullptr;
  auto parents = mContext->getParents(*s);
  while (true) {
    if (stmt != nullptr) {
      parents = mContext->getParents(*stmt);
    }

    if (decl != nullptr) {
      parents = mContext->getParents(*decl);
    }

    if (parents.empty()) {
      return "";
    }

    const auto stmtParent = parents[0].get<clang::Stmt>();
    const auto declParent = parents[0].get<clang::Decl>();
    if (stmtParent == nullptr) {
      if (declParent == nullptr) {
        break;
      }

      stmt = nullptr;
      decl = declParent;
      if (const auto fd = clang::dyn_cast<clang::FunctionDecl>(decl)) {
        return fd->getQualifiedNameAsString();
      }
    } else {
      stmt = stmtParent;
      decl = nullptr;
    }
  }

  return "";
}

bool MutationOperator::isValidMutantSourceRange(
    clang::SourceLocation *startLoc, clang::SourceLocation *endLoc) {
  if (startLoc->isInvalid() || endLoc->isInvalid()) {
    return false;
  }

  if (startLoc->isMacroID() || endLoc->isMacroID()) {
    return false;
  }

  if (mSrcMgr.getMainFileID() != mSrcMgr.getFileID(*startLoc) ||
      mSrcMgr.getMainFileID() != mSrcMgr.getFileID(*endLoc)) {
    return false;
  }

  return true;
}

}  // namespace sentinel

