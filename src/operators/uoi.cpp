/*
  MIT License

  Copyright (c) 2020 LG Electronics, Inc.

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

#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/Lex/Lexer.h>
#include <iostream>
#include <string>
#include "sentinel/operators/uoi.hpp"


namespace sentinel {

bool UOI::canMutate(clang::Stmt* s) {
  auto e = clang::dyn_cast<clang::Expr>(s);
  if (e == nullptr) {
    return false;
  }

  // UOI targets variable reference expression, which includes
  // a variable reference, member reference (class.member, class->member),
  // array subscript expression (arr[idx]), pointer dereference (*ptr)
  if (!(isDeclRefExpr(e) ||
        isPointerDereferenceExpr(e) ||
        clang::isa<clang::ArraySubscriptExpr>(e) ||
        clang::isa<clang::MemberExpr>(e))) {
    return false;
  }

  // UOI targets non-constant, arithmetic/boolean expression only.
  if (e->getType().isConstant(*mContext) ||
      (!getExprType(e)->isArithmeticType() &&
       !getExprType(e)->isBooleanType())) {
    return false;
  }

  // UOI must not be applied to expression that is temporary object.
  // Ex: foo().member
  //     foo() is a function returning a structure-type temporary object.
  //     Member of temporary object cannot be incremented.
  if (auto me = clang::dyn_cast<clang::MemberExpr>(e)) {
    if (clang::isa<clang::MaterializeTemporaryExpr>(me->getBase())) {
      return false;
    }
  }

  // UOI must not be applied to variable reference expression that is:
  // 1. left hand side of assignment expression, or
  // 2. operand of address-of operator (&), or
  // 3. in return statment, or
  // 4. inside lambda capture
  const clang::Stmt* parent = getParentStmt(s);
  while (parent != nullptr) {
    if (auto bo = clang::dyn_cast<clang::BinaryOperator>(parent)) {
      if (bo->isAssignmentOp()) {
        if (bo->getLHS()->IgnoreParens() == s) {
          return false;
        }
      }
    }

    if (auto uo = clang::dyn_cast<clang::UnaryOperator>(parent)) {
      if (uo->getOpcode() == clang::UO_AddrOf ||
          uo->isIncrementDecrementOp()) {
        if (uo->getSubExpr()->IgnoreParens() == s) {
          return false;
        }
      }
    }

    if (clang::isa<clang::ReturnStmt>(parent)) {
      return false;
    }

    if (auto le = clang::dyn_cast<clang::LambdaExpr>(parent)) {
      clang::SourceRange range = le->getIntroducerRange();
      clang::SourceLocation varLoc = e->getBeginLoc();
      if (varLoc.getRawEncoding() >= range.getBegin().getRawEncoding() &&
          varLoc.getRawEncoding() <= range.getEnd().getRawEncoding()) {
        return false;
      }
    }

    parent = getParentStmt(parent);
  }

  return true;
}

void UOI::populate(clang::Stmt* s, Mutants* mutables) {
  auto e = clang::dyn_cast<clang::Expr>(s);
  clang::SourceLocation stmtStartLoc = e->getBeginLoc();
  clang::SourceLocation stmtEndLoc = clang::Lexer::getLocForEndOfToken(
      e->getEndLoc(), 0, mSrcMgr, mContext->getLangOpts());

  if (!isValidMutantSourceRange(&stmtStartLoc, &stmtEndLoc)) {
    return;
  }

  std::string path{mSrcMgr.getFilename(stmtStartLoc)};
  std::string func = getContainingFunctionQualifiedName(s);
  std::string stmtStr = convertStmtToString(e);

  if (getExprType(e)->isArithmeticType() &&
      !getExprType(e)->isBooleanType()) {
    mutables->emplace_back(
        mName, path, func,
        mSrcMgr.getExpansionLineNumber(stmtStartLoc),
        mSrcMgr.getExpansionColumnNumber(stmtStartLoc),
        mSrcMgr.getExpansionLineNumber(stmtEndLoc),
        mSrcMgr.getExpansionColumnNumber(stmtEndLoc),
        "((" + stmtStr + ")++)");

    mutables->emplace_back(
        mName, path, func,
        mSrcMgr.getExpansionLineNumber(stmtStartLoc),
        mSrcMgr.getExpansionColumnNumber(stmtStartLoc),
        mSrcMgr.getExpansionLineNumber(stmtEndLoc),
        mSrcMgr.getExpansionColumnNumber(stmtEndLoc),
        "((" + stmtStr + ")--)");
  }

  if (getExprType(e)->isBooleanType()) {
    mutables->emplace_back(
        mName, path, func,
        mSrcMgr.getExpansionLineNumber(stmtStartLoc),
        mSrcMgr.getExpansionColumnNumber(stmtStartLoc),
        mSrcMgr.getExpansionLineNumber(stmtEndLoc),
        mSrcMgr.getExpansionColumnNumber(stmtEndLoc),
        "(!(" + stmtStr + "))");
  }
}

}  // namespace sentinel
