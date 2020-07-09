#include "clang/AST/Expr.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/ParentMapContext.h"

#include "sbr.h"
#include "lemon_utility.h"

using namespace std;
using namespace clang;

// ISSUE: Deleting a label.
bool SBR::canMutate(
    clang::Stmt *s, Configuration *config, CompilerInstance &comp_inst) {
  // Declarations and null statements are not mutated
  if (isa<DeclStmt>(s) || isa<NullStmt>(s) || isa<CompoundStmt>(s))
    return false;

  // Only mutate statements starting at a targeted line.
  int start_line = getLineNumber(
      comp_inst.getSourceManager(),
      getPreciseLocation(s->getBeginLoc(), comp_inst, true));

  if (!config->isTargetLine(start_line))
    return false;

  // Only delete COMPLETE statements.
  if (!isCompleteStmt(s, comp_inst))
    return false;

  return true;  
}

// ISSUE: delete compound statement? if for while do body?
void SBR::mutate(clang::Stmt *s, MutantDatabase *database) {
  SourceManager &src_mgr = database->getCompilerInstance().getSourceManager();
  SourceLocation start_loc = getPreciseLocation(
      s->getBeginLoc(), database->getCompilerInstance(), true);
  SourceLocation end_loc = getPreciseLocation(
      s->getEndLoc(), database->getCompilerInstance(), false);
  string token = getCodeInSourceRange(start_loc, end_loc, src_mgr);
  string mutated_token = "";
  int target_line = getLineNumber(src_mgr, start_loc);
  
  database->addMutantEntry(
      name, start_loc, end_loc, token, mutated_token, target_line);
}

const clang::Stmt* SBR::getImmediateParentCompoundStmt(
    const clang::Stmt *stmt_ptr, clang::CompilerInstance &comp_inst)
{
  // const Stmt* stmt_ptr = s;

  const auto parent_stmt = comp_inst.getASTContext().getParents(*stmt_ptr);
  if (parent_stmt.empty())
    return nullptr;
  
  stmt_ptr = parent_stmt[0].get<Stmt>();
  if (!stmt_ptr)
    return nullptr;

  return stmt_ptr;
}

bool SBR::isCompleteStmt(clang::Stmt *s, clang::CompilerInstance &comp_inst) {
  // We detect complete stmt by checking if parent is a CompoundStmt.
  const Stmt* parent = getImmediateParentCompoundStmt(s, comp_inst);
  if (!parent)
    return false;

  if (const IfStmt *is = dyn_cast<IfStmt>(parent)) {
    if (is->getThen() == s || is->getElse() == s)
      return true;
  }
  else if (const ForStmt *fs = dyn_cast<ForStmt>(parent)) {
    if (fs->getBody() == s)
      return true;
  }
  else if (const DoStmt *ds = dyn_cast<DoStmt>(parent)) {
    if (ds->getBody() == s)
      return true;
  }
  else if (const WhileStmt *ws = dyn_cast<WhileStmt>(parent)) {
    if (ws->getBody() == s)
      return true;
  }

  if (!isa<CompoundStmt>(parent))
    return false;

  // The last statement of a Statement Expression cannot be deleted.
  // Because it is the value of the expression.
  auto cs = cast<CompoundStmt>(parent);
  if (isLastStmtOfStmtExpr(s, cs, comp_inst))
    return false;

  return true;
}

bool SBR::isLastStmtOfStmtExpr(
    clang::Stmt *s, const CompoundStmt *cs, CompilerInstance &comp_inst) {
  const Stmt* parent = getImmediateParentCompoundStmt(s, comp_inst);
  if (!parent)
    return false;

  const Stmt *second_level_parent = getImmediateParentCompoundStmt(
      parent, comp_inst);
  if (!second_level_parent || !isa<StmtExpr>(second_level_parent))
    return false;

  // find the targeted statement 
  auto it = cs->body_begin();
  for (; it != cs->body_end(); it++) {
    if (*it == s)
      break;
  }

  // Return True if this is the last stmt
  ++it;
  if (it == cs->body_end())
    return true;

  return false;
}
