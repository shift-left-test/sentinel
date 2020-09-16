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

#include <string>
#include "clang/AST/Expr.h"
#include "clang/Lex/Lexer.h"
#include "sentinel/operators/uoi.hpp"
#include "sentinel/util/astnode.hpp"


namespace sentinel {

bool UOI::canMutate(clang::Stmt* s) {
  auto e = clang::dyn_cast<clang::Expr>(s);
  if (e == nullptr) {
    return false;
  }

  // UOI targets variable reference expression, which includes
  // a variable reference, member reference (class.member, class->member),
  // array subscript expression (arr[idx]), pointer dereference (*ptr)
  if (!(util::astnode::isDeclRefExpr(e) ||
        util::astnode::isPointerDereferenceExpr(e) ||
        clang::isa<clang::ArraySubscriptExpr>(e) ||
        clang::isa<clang::MemberExpr>(e))) {
    return false;
  }

  // UOI targets non-constant, arithmetic/boolean expression only.
  return !e->getType().isConstant(mCI.getASTContext()) &&
         ((util::astnode::getExprType(e)->isScalarType() &&
           !util::astnode::getExprType(e)->isPointerType()) ||
          (util::astnode::getExprType(e)->isBooleanType()));
}

void UOI::populate(clang::Stmt* s, Mutables* mutables) {
  auto e = clang::dyn_cast<clang::Expr>(s);
  clang::SourceLocation stmtStartLoc = e->getBeginLoc();
  clang::SourceLocation stmtEndLoc = clang::Lexer::getLocForEndOfToken(
      e->getEndLoc(), 0, mSrcMgr, mCI.getLangOpts());
  std::string path = mSrcMgr.getFilename(stmtStartLoc);
  std::string func = util::astnode::getContainingFunctionQualifiedName(s, mCI);
  std::string stmtStr = util::astnode::convertStmtToString(e, mCI);

  if (stmtStartLoc.isMacroID() || stmtEndLoc.isMacroID()) {
    return;
  }

  if (util::astnode::getExprType(e)->isScalarType() &&
      !util::astnode::getExprType(e)->isPointerType()) {
    mutables->add(Mutable("UOI", path, func,
                          mSrcMgr.getExpansionLineNumber(stmtStartLoc),
                          mSrcMgr.getExpansionColumnNumber(stmtStartLoc),
                          mSrcMgr.getExpansionLineNumber(stmtEndLoc),
                          mSrcMgr.getExpansionColumnNumber(stmtEndLoc),
                          "((" + stmtStr + ")++)"));

    mutables->add(Mutable("UOI", path, func,
                          mSrcMgr.getExpansionLineNumber(stmtStartLoc),
                          mSrcMgr.getExpansionColumnNumber(stmtStartLoc),
                          mSrcMgr.getExpansionLineNumber(stmtEndLoc),
                          mSrcMgr.getExpansionColumnNumber(stmtEndLoc),
                          "((" + stmtStr + ")--)"));
  }

  if (util::astnode::getExprType(e)->isBooleanType()) {
    mutables->add(Mutable("UOI", path, func,
                          mSrcMgr.getExpansionLineNumber(stmtStartLoc),
                          mSrcMgr.getExpansionColumnNumber(stmtStartLoc),
                          mSrcMgr.getExpansionLineNumber(stmtEndLoc),
                          mSrcMgr.getExpansionColumnNumber(stmtEndLoc),
                          "(!(" + stmtStr + "))"));
  }
}

}  // namespace sentinel
