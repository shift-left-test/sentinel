#include "clang/AST/Expr.h"

#include "aor.h"
#include "lemon_utility.h"

using namespace std;
using namespace clang;

set<string> arithmetic_operators{"+", "-", "*", "/", "%"};

bool AOR::canMutate(
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

  // Return true if the binary operator used is a logical operator.
  return arithmetic_operators.find(binary_operator) != arithmetic_operators.end();

  // SourceLocation start_loc = bo->getOperatorLoc();
  // SourceManager &src_mgr = comp_inst.getSourceManager();

  // SourceLocation end_loc = src_mgr.translateLineCol(
  //     src_mgr.getMainFileID(),
  //     getLineNumber(src_mgr, start_loc),
  //     getColumnNumber(src_mgr, start_loc) + binary_operator.length());
  
  

  // StmtContext &stmt_context = context->getStmtContext();
  // return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
  //        !stmt_context.IsInArrayDeclSize() &&
  //        !stmt_context.IsInEnumDecl() &&
  //        domain_.find(binary_operator) != domain_.end();
}

void AOR::mutate(clang::Stmt *s, MutantDatabase *database) {
  BinaryOperator *bo;
  if (!(bo = dyn_cast<BinaryOperator>(s)))
    return;

  int target_line = getLineNumber(
      database->getCompilerInstance().getSourceManager(),
      getPreciseLocation(
          bo->getOperatorLoc(), database->getCompilerInstance(), true));

  mutateOperator(bo, database, target_line);
  mutateWholeExpr(bo, database, target_line);
}

void AOR::mutateOperator(
    clang::BinaryOperator *bo, MutantDatabase *database, int target_line) {
  if (bo->getOperatorLoc().isMacroID())
    return;

  CompilerInstance &comp_inst = database->getCompilerInstance();
  // SourceManager &src_mgr = database->getCompilerInstance().getSourceManager();
  
  string targeted_operator{bo->getOpcodeStr()};
  SourceLocation start_loc = getPreciseLocation(
      bo->getBeginLoc(), comp_inst, true);
  SourceLocation end_loc = getPreciseLocation(
      bo->getEndLoc(), comp_inst, false);
  // SourceLocation start_loc = bo->getOperatorLoc();
  // SourceLocation end_loc = src_mgr.translateLineCol(
  //     src_mgr.getMainFileID(), getLineNumber(src_mgr, start_loc),
  //     getColumnNumber(src_mgr, start_loc) + targeted_operator.length());

  for (auto mutated_operator: arithmetic_operators) {
    if (violateCSyntax(bo, mutated_operator, database->getCompilerInstance()) ||
        targeted_operator.compare(mutated_operator) == 0)
      continue;

    string token = convertStmtToString(bo, comp_inst);
    string lhs = convertStmtToString(bo->getLHS(), comp_inst);
    string rhs = convertStmtToString(bo->getRHS(), comp_inst);
    string mutated_token = "((" + lhs + ") " + mutated_operator + \
                           " (" + rhs + "))"; 
    
    database->addMutantEntry(
        name, start_loc, end_loc, token, mutated_token, target_line);
  }
}

void AOR::mutateWholeExpr(
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

bool AOR::violateCSyntax(
    clang::BinaryOperator *bo, std::string mutated_operator, 
    clang::CompilerInstance &comp_inst) {
  Expr *lhs = bo->getLHS()->IgnoreImpCasts();
  Expr *rhs = bo->getRHS()->IgnoreImpCasts();
  
  // Modular operator only takes integral operands
  if (mutated_operator.compare("%") == 0) {
    return !lhs->getType().getCanonicalType().getTypePtr()->isIntegralType(
               comp_inst.getASTContext()) ||
           !rhs->getType().getCanonicalType().getTypePtr()->isIntegralType(
               comp_inst.getASTContext());
  }

  // Multiply and Divide operators cannot take pointer-type operands
  if (mutated_operator.compare("*") == 0 || mutated_operator.compare("/") == 0) {
    return isPointerType(lhs) || isArrayType(lhs) ||
           isPointerType(rhs) || isArrayType(rhs); 
  }

  // The followinng pointer arithmetic operations are allowed
  // (ptr + int) (ptr - int) (ptr - ptr) (int + ptr)
  // if (isPointerType(rhs) || isArrayType(rhs))
  //   return false;

  return false;
}

