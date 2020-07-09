#include "clang/AST/Expr.h"

#include "sor.h"
#include "lemon_utility.h"

using namespace std;
using namespace clang;

set<string> shift_operators{">>", "<<"};
set<string> shift_assignments{">>=", "<<="};

bool SOR::canMutate(
    clang::Stmt *s, Configuration *config, CompilerInstance &comp_inst) {
  BinaryOperator *bo = dyn_cast<BinaryOperator>(s);

  // if this statement does not contain binary operator --> SKIP
  if (!bo)
    return false;

  string binary_operator{bo->getOpcodeStr()};

  int start_line = getLineNumber(
      comp_inst.getSourceManager(),
      getPreciseLocation(bo->getOperatorLoc(), comp_inst, true));

  if (!config->isTargetLine(start_line))
    return false;

  // SourceLocation start_loc = bo->getOperatorLoc();
  // SourceManager &src_mgr = comp_inst.getSourceManager();

  // SourceLocation end_loc = src_mgr.translateLineCol(
  //     src_mgr.getMainFileID(),
  //     getLineNumber(src_mgr, start_loc),
  //     getColumnNumber(src_mgr, start_loc) + binary_operator.length());
  
  // Return true if the binary operator used is a logical operator.
  return shift_operators.find(binary_operator) != shift_operators.end() ||
         shift_assignments.find(binary_operator) != shift_assignments.end();

  // StmtContext &stmt_context = context->getStmtContext();
  // return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
  //        !stmt_context.IsInArrayDeclSize() &&
  //        !stmt_context.IsInEnumDecl() &&
  //        domain_.find(binary_operator) != domain_.end();
}

void SOR::mutate(clang::Stmt *s, MutantDatabase *database) {
  BinaryOperator *bo;
  if (!(bo = dyn_cast<BinaryOperator>(s)))
    return;

  int target_line = getLineNumber(
      database->getCompilerInstance().getSourceManager(),
      getPreciseLocation(
          bo->getOperatorLoc(), database->getCompilerInstance(), true));

  string token{bo->getOpcodeStr()};
  if (token.length() > 2)
    token = token.substr(0, token.length()-1);
  string mutated_token{""};
  SourceManager &src_mgr = database->getCompilerInstance().getSourceManager();
  SourceLocation start_loc = bo->getOperatorLoc();
  SourceLocation end_loc = src_mgr.translateLineCol(
      src_mgr.getMainFileID(),
      getLineNumber(src_mgr, start_loc),
      getColumnNumber(src_mgr, start_loc) + token.length());  
  database->addMutantTarget(
        name, start_loc, end_loc, token, mutated_token, target_line);

  if (shift_assignments.find(token) != shift_assignments.end())
    return;

  mutateOperator(bo, database, target_line);
  mutateWholeExpr(bo, database, target_line);
}

void SOR::mutateOperator(
    clang::BinaryOperator *bo, MutantDatabase *database, int target_line) {
  string token{bo->getOpcodeStr()};
  SourceLocation start_loc = bo->getOperatorLoc();

  if (start_loc.isMacroID())
    return;

  SourceManager &src_mgr = database->getCompilerInstance().getSourceManager();
  
  SourceLocation end_loc = src_mgr.translateLineCol(
      src_mgr.getMainFileID(),
      getLineNumber(src_mgr, start_loc),
      getColumnNumber(src_mgr, start_loc) + token.length());

  for (auto mutated_token: shift_operators)
    if (token.compare(mutated_token) != 0) {
      database->addMutantEntry(
          name, start_loc, end_loc, token, mutated_token, target_line);
    }
}

void SOR::mutateWholeExpr(
    clang::BinaryOperator *bo, MutantDatabase *database, int target_line) {
  SourceManager &src_mgr = database->getCompilerInstance().getSourceManager();
  if (sourceRangeIsInSameMacro(bo->getBeginLoc(), bo->getEndLoc(), src_mgr))
    return;

  SourceLocation start_loc = getPreciseLocation(
      bo->getBeginLoc(), database->getCompilerInstance(), true);
  SourceLocation end_loc = getPreciseLocation(
      bo->getEndLoc(), database->getCompilerInstance(), false);

  string token = convertStmtToString(bo, database->getCompilerInstance());

  // mutate to left hand side
  string lhs = convertStmtToString(bo->getLHS(), 
                                   database->getCompilerInstance());
  database->addMutantEntry(name, start_loc, end_loc, token, lhs, target_line);

  // mutate to right hand side
  string rhs = convertStmtToString(bo->getRHS(), 
                                   database->getCompilerInstance());
  database->addMutantEntry(name, start_loc, end_loc, token, rhs, target_line);  
}



