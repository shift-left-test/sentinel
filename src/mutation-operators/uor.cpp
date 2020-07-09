#include "clang/AST/Expr.h"

#include "uor.h"
#include "lemon_utility.h"

using namespace std;
using namespace clang;

bool UOR::canMutate(
    clang::Stmt *s, Configuration *config, CompilerInstance &comp_inst) {
  if (UnaryOperator *uo = dyn_cast<UnaryOperator>(s))
    if (uo->getOpcode() == UO_PostDec || uo->getOpcode() == UO_PreDec ||
        uo->getOpcode() == UO_PostInc || uo->getOpcode() == UO_PreInc ||
        uo->getOpcode() == UO_LNot) {
      int start_line = getLineNumber(
          comp_inst.getSourceManager(),
          getPreciseLocation(uo->getOperatorLoc(), comp_inst, true));

      return config->isTargetLine(start_line);
    }

  return false;
}

void UOR::mutate(clang::Stmt *s, MutantDatabase *database) {
  UnaryOperator *uo;
  if (!(uo = dyn_cast<UnaryOperator>(s)))
    return;

  int target_line = getLineNumber(
      database->getCompilerInstance().getSourceManager(),
      getPreciseLocation(
          uo->getOperatorLoc(), database->getCompilerInstance(), true));

  string token;
  if (uo->getOpcode() == UO_PostDec || uo->getOpcode() == UO_PreDec)
    token = "--";
  else 
    if (uo->getOpcode() == UO_PostInc || uo->getOpcode() == UO_PreInc)
      token = "++";
    else
      token = "!";

  string mutated_token{""};
  SourceManager &src_mgr = database->getCompilerInstance().getSourceManager();
  SourceLocation start_loc = uo->getOperatorLoc();
  SourceLocation end_loc = src_mgr.translateLineCol(
      src_mgr.getMainFileID(),
      getLineNumber(src_mgr, start_loc),
      getColumnNumber(src_mgr, start_loc) + token.length());  
  database->addMutantTarget(
        name, start_loc, end_loc, token, mutated_token, target_line);
}



