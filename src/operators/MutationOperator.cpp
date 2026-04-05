/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <clang/AST/ASTContext.h>
#if LLVM_VERSION_MAJOR >= 11
#include <clang/AST/ParentMapContext.h>
#endif
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Lex/Lexer.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "sentinel/operators/MutationOperator.hpp"
#include "sentinel/util/string.hpp"
#include "sentinel/operators/aor.hpp"
#include "sentinel/operators/bor.hpp"
#include "sentinel/operators/lcr.hpp"
#include "sentinel/operators/ror.hpp"
#include "sentinel/operators/sdl.hpp"
#include "sentinel/operators/sor.hpp"
#include "sentinel/operators/uoi.hpp"

namespace sentinel {

MutationOperator::~MutationOperator() = default;

std::string MutationOperator::convertStmtToString(const clang::Stmt* s) {
  clang::SourceLocation walkLoc = s->getBeginLoc();
  clang::SourceLocation endLoc =
      clang::Lexer::getLocForEndOfToken(s->getEndLoc(), 0, mContext->getSourceManager(), mContext->getLangOpts());
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

  return parent[0].get<clang::Stmt>();
}

const clang::Type* MutationOperator::getExprType(clang::Expr* e) {
  return e->getType().getCanonicalType().getTypePtr();
}

bool MutationOperator::isDeclRefExpr(clang::Stmt* s) {
  if (auto dre = clang::dyn_cast<clang::DeclRefExpr>(s)) {
    auto type = getExprType(dre);
    if (type->isEnumeralType()) {
      return false;
    }
    return !clang::isa<clang::EnumConstantDecl>(dre->getDecl());
  }

  return false;
}

bool MutationOperator::isPointerDereferenceExpr(clang::Stmt* s) {
  if (auto uo = clang::dyn_cast<clang::UnaryOperator>(s)) {
    return (uo->getOpcode() == clang::UO_Deref);
  }

  return false;
}

std::string MutationOperator::getContainingFunctionQualifiedName(clang::Stmt* s) {
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

bool MutationOperator::isValidMutantSourceRange(clang::SourceLocation* startLoc, clang::SourceLocation* endLoc) {
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

bool MutationOperator::isLoopCondition(const clang::Stmt* s) {
  const clang::Stmt* child = s;
  const clang::Stmt* parent = getParentStmt(s);

  while (parent != nullptr) {
    if (auto fs = clang::dyn_cast<clang::ForStmt>(parent)) {
      if (fs->getCond() == child) {
        return true;
      }
    } else if (auto ws = clang::dyn_cast<clang::WhileStmt>(parent)) {
      if (ws->getCond() == child) {
        return true;
      }
    } else if (auto ds = clang::dyn_cast<clang::DoStmt>(parent)) {
      if (ds->getCond() == child) {
        return true;
      }
    }

    child = parent;
    parent = getParentStmt(parent);
  }

  return false;
}

bool MutationOperator::getIntegerLiteralValue(const clang::Expr* e, int64_t* value) {
  const clang::Expr* stripped = e->IgnoreImpCasts();

  if (auto il = clang::dyn_cast<clang::IntegerLiteral>(stripped)) {
    *value = il->getValue().getSExtValue();
    return true;
  }

  if (auto bl = clang::dyn_cast<clang::CXXBoolLiteralExpr>(stripped)) {
    *value = bl->getValue() ? 1 : 0;
    return true;
  }

  return false;
}

void MutationOperator::populateBinaryReplacements(
    clang::BinaryOperator* bo, clang::Stmt* s,
    const std::set<std::string>& operators, Mutants* mutables,
    const std::function<bool(const std::string&)>& filter) {
  std::string token{bo->getOpcodeStr()};
  clang::SourceLocation opStartLoc = bo->getOperatorLoc();
  clang::SourceLocation opEndLoc =
      mSrcMgr.translateLineCol(mSrcMgr.getMainFileID(), mSrcMgr.getExpansionLineNumber(opStartLoc),
                               mSrcMgr.getExpansionColumnNumber(opStartLoc) + token.length());

  if (!isValidMutantSourceRange(&opStartLoc, &opEndLoc)) {
    return;
  }

  std::string path{mSrcMgr.getFilename(opStartLoc)};
  std::string func = getContainingFunctionQualifiedName(s);

  for (const auto& mutatedToken : operators) {
    if (mutatedToken == token) {
      continue;
    }
    if (filter && !filter(mutatedToken)) {
      continue;
    }
    mutables->emplace_back(mName, path, func, mSrcMgr.getExpansionLineNumber(opStartLoc),
                           mSrcMgr.getExpansionColumnNumber(opStartLoc), mSrcMgr.getExpansionLineNumber(opEndLoc),
                           mSrcMgr.getExpansionColumnNumber(opEndLoc), mutatedToken);
  }
}

void resolveExpansionLineRange(clang::Stmt* s, clang::SourceManager* srcMgr,
                               const clang::LangOptions& langOpts,
                               std::size_t* startLineNum,
                               std::size_t* endLineNum) {
  clang::SourceLocation startLoc = s->getBeginLoc();
  clang::SourceLocation endLoc = s->getEndLoc();

  if (startLoc.isMacroID()) {
    clang::CharSourceRange range =
        srcMgr->getImmediateExpansionRange(startLoc);
    startLoc = range.getBegin();
  }

  if (endLoc.isMacroID()) {
    clang::CharSourceRange range =
        srcMgr->getImmediateExpansionRange(endLoc);
    endLoc = clang::Lexer::getLocForEndOfToken(
        range.getEnd(), 0, *srcMgr, langOpts);
  }

  *startLineNum = srcMgr->getExpansionLineNumber(startLoc);
  *endLineNum = srcMgr->getExpansionLineNumber(endLoc);
}

std::vector<std::unique_ptr<MutationOperator>> createOperators(
    clang::ASTContext* context, const std::vector<std::string>& selectedOps) {
  std::vector<std::string> normalizedOps(selectedOps.size());
  std::transform(selectedOps.begin(), selectedOps.end(), normalizedOps.begin(),
                 [](const std::string& s) { return string::toUpper(s); });
  auto include = [&normalizedOps](const std::string& name) {
    return normalizedOps.empty() ||
           std::find(normalizedOps.begin(), normalizedOps.end(), name) != normalizedOps.end();
  };
  std::vector<std::unique_ptr<MutationOperator>> ops;
  if (include("AOR")) ops.push_back(std::make_unique<AOR>(context));
  if (include("BOR")) ops.push_back(std::make_unique<BOR>(context));
  if (include("ROR")) ops.push_back(std::make_unique<ROR>(context));
  if (include("SOR")) ops.push_back(std::make_unique<SOR>(context));
  if (include("LCR")) ops.push_back(std::make_unique<LCR>(context));
  if (include("SDL")) ops.push_back(std::make_unique<SDL>(context));
  if (include("UOI")) ops.push_back(std::make_unique<UOI>(context));
  return ops;
}

}  // namespace sentinel
