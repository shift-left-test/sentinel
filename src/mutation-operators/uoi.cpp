#include "clang/AST/Expr.h"

#include "uoi.h"
#include "lemon_utility.h"

using namespace std;
using namespace clang;

bool UOI::canMutate(
    clang::Stmt *s, Configuration *config, CompilerInstance &comp_inst) {
  // The target of UOI must be refering/pointing to a variable 
  // (variable reference expression), 
  // and it must be of type arithmetic or boolean.
  if (!isVariableReferenceExpr(s))
    return false;

  int start_line = getLineNumber(
      comp_inst.getSourceManager(),
      getPreciseLocation(s->getBeginLoc(), comp_inst, true));

  if (!config->isTargetLine(start_line))
    return false;

  Expr *e = dyn_cast<Expr>(s);
  if (!e) 
    return false;

  // Do not attempt to mutate variable in MACRO.
  // Certain cases are possible (e.g. #define macro_name variable_name).
  // Ignore for complication issue. --> May resolve later if necessary
  SourceManager &src_mgr = comp_inst.getSourceManager();
  if (sourceRangeIsInSameMacro(e->getBeginLoc(), e->getEndLoc(), src_mgr))
    return false;

  return !e->getType().isConstant(comp_inst.getASTContext()) && 
         (isArithmeticType(e) || isBooleanType(e));

  // StmtContext &stmt_context = context->getStmtContext();
  // return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
  //        !stmt_context.IsInArrayDeclSize() &&
  //        !stmt_context.IsInEnumDecl() &&
  //        domain_.find(binary_operator) != domain_.end();
}

void UOI::mutate(clang::Stmt *s, MutantDatabase *database) {
  Expr *e;
  if (!(e = dyn_cast<Expr>(s)))
    return;

  SourceLocation start_loc = getPreciseLocation(
      e->getBeginLoc(), database->getCompilerInstance(), true);
  SourceLocation end_loc = getPreciseLocation(
      e->getEndLoc(), database->getCompilerInstance(), false);
  string token = convertStmtToString(e, database->getCompilerInstance());
  int target_line = getLineNumber(
      database->getCompilerInstance().getSourceManager(), start_loc);

  if (isArithmeticType(e)) {
    string mutated_token = "((" + token + ")++)";
    database->addMutantEntry(
        name, start_loc, end_loc, token, mutated_token, target_line);

    mutated_token = "((" + token + ")--)";
    database->addMutantEntry(
        name, start_loc, end_loc, token, mutated_token, target_line);
  }
  else if (isBooleanType(e)) {
    string mutated_token = "(!(" + token + "))";
    database->addMutantEntry(
        name, start_loc, end_loc, token, mutated_token, target_line);
  }
}
