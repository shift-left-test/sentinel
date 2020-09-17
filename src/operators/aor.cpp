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
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/Expr.h"
#include "clang/Lex/Lexer.h"
#include "sentinel/operators/aor.hpp"
#include "sentinel/util/astnode.hpp"

namespace sentinel {

bool AOR::canMutate(clang::Stmt* s) {
  auto bo = clang::dyn_cast<clang::BinaryOperator>(s);
  if (bo == nullptr) {
    return false;
  }

  return mArithmeticOperators.find(bo->getOpcodeStr()) != \
         mArithmeticOperators.end();
}

void AOR::populate(clang::Stmt* s, Mutables* mutables) {
  auto bo = clang::dyn_cast<clang::BinaryOperator>(s);

  // create mutables from the operator
  std::string token{bo->getOpcodeStr()};
  clang::SourceLocation opStartLoc = bo->getOperatorLoc();
  clang::SourceLocation opEndLoc = mSrcMgr.translateLineCol(
      mSrcMgr.getMainFileID(),
      mSrcMgr.getExpansionLineNumber(opStartLoc),
      mSrcMgr.getExpansionColumnNumber(opStartLoc) + token.length());

  if (!opStartLoc.isMacroID() && !opEndLoc.isMacroID()) {
    std::string path = mSrcMgr.getFilename(opStartLoc);
    std::string func = astnode::getContainingFunctionQualifiedName(s,
      mContext);

    for (const auto& mutatedToken : mArithmeticOperators) {
      if (mutatedToken == token) {
        continue;
      }

      clang::Expr *lhs = bo->getLHS()->IgnoreImpCasts();
      clang::Expr *rhs = bo->getRHS()->IgnoreImpCasts();

      // modulo operator only takes integral operands.
      if (mutatedToken == "%" &&
          (!astnode::getExprType(lhs)->isIntegralType(mContext) ||
           !astnode::getExprType(rhs)->isIntegralType(mContext))) {
        continue;
      }

      if ((mutatedToken == "*" || mutatedToken == "/") &&
          (astnode::getExprType(lhs)->isPointerType() ||
           astnode::getExprType(rhs)->isPointerType() ||
           astnode::getExprType(lhs)->isArrayType() ||
           astnode::getExprType(rhs)->isArrayType())) {
        continue;
      }

      mutables->add(Mutable(mName, path, func,
        mSrcMgr.getExpansionLineNumber(opStartLoc),
        mSrcMgr.getExpansionColumnNumber(opStartLoc),
        mSrcMgr.getExpansionLineNumber(opEndLoc),
        mSrcMgr.getExpansionColumnNumber(opEndLoc),
        mutatedToken));
    }
  }
}

}  // namespace sentinel
