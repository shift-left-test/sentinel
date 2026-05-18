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
  if (clang::isa<clang::DeclStmt>(s) || clang::isa<clang::NullStmt>(s) || clang::isa<clang::IfStmt>(s) ||
      clang::isa<clang::ForStmt>(s) || clang::isa<clang::CXXForRangeStmt>(s) || clang::isa<clang::DoStmt>(s) ||
      clang::isa<clang::WhileStmt>(s) || clang::isa<clang::CompoundStmt>(s) || clang::isa<clang::SwitchStmt>(s) ||
      clang::isa<clang::CXXTryStmt>(s) || clang::isa<clang::CXXDeleteExpr>(s) || clang::isa<clang::ReturnStmt>(s) ||
      clang::isa<clang::BreakStmt>(s) || clang::isa<clang::ContinueStmt>(s)) {
    return false;
  }

  // Apply SDL to body of if, for, do, while if it is a single statement
  // (instead of compound statement)
  const clang::Stmt* parent = getParentStmt(s);
  if (parent == nullptr) {
    return false;
  }

  if (auto ifStmt = clang::dyn_cast<clang::IfStmt>(parent);
      ifStmt != nullptr && (ifStmt->getThen() == s || ifStmt->getElse() == s)) {
    return true;
  }
  if (auto forStmt = clang::dyn_cast<clang::ForStmt>(parent);
      forStmt != nullptr && forStmt->getBody() == s) {
    return true;
  }
  if (auto doStmt = clang::dyn_cast<clang::DoStmt>(parent);
      doStmt != nullptr && doStmt->getBody() == s) {
    return true;
  }
  if (auto whileStmt = clang::dyn_cast<clang::WhileStmt>(parent);
      whileStmt != nullptr && whileStmt->getBody() == s) {
    return true;
  }

  // Apply SDL to immediate child node of CompoundStmt node
  if (!clang::isa<clang::CompoundStmt>(parent)) {
    return false;
  }

  // The last statement of a Statement Expression should not be deleted.
  // Because it is the value of the expression.
  auto cs = clang::dyn_cast<clang::CompoundStmt>(parent);
  const clang::Stmt* parentOfParent = getParentStmt(parent);
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
  //
  // getCharacterData() returns a pointer into the file buffer and does not
  // signal EOF, so the scan must be bounded by the FileID's buffer end —
  // otherwise a missing terminator (e.g. last statement of a file with no
  // trailing newline) reads past the buffer.
  bool invalid = false;
  auto buf = mSrcMgr.getBufferData(mSrcMgr.getFileID(stmtEndLoc), &invalid);
  if (invalid) {
    return;
  }
  const char* bufEnd = buf.data() + buf.size();
  auto semiLoc = stmtEndLoc;
  auto c = mSrcMgr.getCharacterData(semiLoc);

  while (c < bufEnd) {
    if (*c == ';') {
      stmtEndLoc = semiLoc.getLocWithOffset(1);
      break;
    }
    if (*c == '\n') {
      break;
    }
    semiLoc = semiLoc.getLocWithOffset(1);
    c = mSrcMgr.getCharacterData(semiLoc);
  }

  if (isValidMutantSourceRange(&stmtStartLoc, &stmtEndLoc)) {
    std::string path{mSrcMgr.getFilename(stmtStartLoc)};
    std::string func = getContainingFunctionQualifiedName(s);

    emitMutant(mutables, path, func, stmtStartLoc, stmtEndLoc, "{}");
  }
}

}  // namespace sentinel
