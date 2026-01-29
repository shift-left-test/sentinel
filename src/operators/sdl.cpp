/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/StmtCXX.h>
#include <clang/Lex/Lexer.h>
#include <string>
#include "sentinel/operators/sdl.hpp"


namespace sentinel {

bool SDL::canMutate(clang::Stmt* s) {
  // Declarations, null, complex  statements are not mutated
  if (clang::isa<clang::DeclStmt>(s) ||
      clang::isa<clang::NullStmt>(s) ||
      clang::isa<clang::IfStmt>(s) ||
      clang::isa<clang::ForStmt>(s) ||
      clang::isa<clang::CXXForRangeStmt>(s) ||
      clang::isa<clang::DoStmt>(s) ||
      clang::isa<clang::WhileStmt>(s) ||
      clang::isa<clang::CompoundStmt>(s) ||
      clang::isa<clang::SwitchStmt>(s) ||
      clang::isa<clang::CXXTryStmt>(s) ||
      clang::isa<clang::CXXDeleteExpr>(s) ||
      clang::isa<clang::ReturnStmt>(s)) {
    return false;
  }

  // Apply SDL to body of if, for, do, while if it is a single statement
  // (instead of compound statement)
  const clang::Stmt* parent = getParentStmt(s);
  if (parent == nullptr) {
    return false;
  }

  if (auto is = clang::dyn_cast<clang::IfStmt>(parent)) {
    if (is->getThen() == s || is->getElse() == s) {
      return true;
    }
  } else {
    if (auto fs = clang::dyn_cast<clang::ForStmt>(parent)) {
      if (fs->getBody() == s) {
        return true;
      }
    } else {
      if (auto ds = clang::dyn_cast<clang::DoStmt>(parent)) {
        if (ds->getBody() == s) {
          return true;
        }
      } else {
        if (auto ws = clang::dyn_cast<clang::WhileStmt>(parent)) {
          if (ws->getBody() == s) {
            return true;
          }
        }
      }
    }
  }

  // Apply SDL to immediate child node of CompoundStmt node
  if (!clang::isa<clang::CompoundStmt>(parent)) {
    return false;
  }

  // The last statement of a Statement Expression should not be deleted.
  // Because it is the value of the expression.
  auto cs = clang::dyn_cast<clang::CompoundStmt>(parent);
  const clang::Stmt *parentOfParent = getParentStmt(parent);
  if (parentOfParent == nullptr || !clang::isa<clang::StmtExpr>(parentOfParent)) {
    return true;
  }

  auto it = cs->body_begin();
  for (; it != cs->body_end(); it++) {
    if (*it == s) {
      break;
    }
  }
  ++it;
  return !(it == cs->body_end());
}

void SDL::populate(clang::Stmt* s, Mutants* mutables) {
  auto stmtStartLoc = s->getBeginLoc();
  auto stmtEndLoc = clang::Lexer::getLocForEndOfToken(s->getEndLoc(), 0, mSrcMgr, mContext->getLangOpts());

  // A temporary solution to get the location after semicolon.
  // Tried Lexer::findLocAfterToken and Lexer::findNextToken
  // but they both do not work.
  auto semiLoc = stmtEndLoc;
  auto c = mSrcMgr.getCharacterData(semiLoc);

  while (true) {
    if (*c == ';') {
      stmtEndLoc = semiLoc.getLocWithOffset(1);
      break;
    }

    if (*c == '\n' || *c == EOF) {
      break;
    }

    semiLoc = semiLoc.getLocWithOffset(1);
    c = mSrcMgr.getCharacterData(semiLoc);
  }

  if (isValidMutantSourceRange(&stmtStartLoc, &stmtEndLoc)) {
    std::string path{mSrcMgr.getFilename(stmtStartLoc)};
    std::string func = getContainingFunctionQualifiedName(s);

    mutables->emplace_back(
        mName, path, func,
        mSrcMgr.getExpansionLineNumber(stmtStartLoc),
        mSrcMgr.getExpansionColumnNumber(stmtStartLoc),
        mSrcMgr.getExpansionLineNumber(stmtEndLoc),
        mSrcMgr.getExpansionColumnNumber(stmtEndLoc),
        "{}");
  }
}

}  // namespace sentinel
