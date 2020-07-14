#include "lemon_ast_visitor.h"
#include "mutation_operators.h"

#include "lemon_utility.h"

using namespace clang;
using namespace std;

LemonASTVisitor::LemonASTVisitor(
    clang::CompilerInstance &comp_inst, MutantDatabase *database)
  : CI(comp_inst), src_mgr(comp_inst.getSourceManager()), database(database),
    config(database->getConfiguration()), isInLhsAssignment(false) {

  // store this to prevent UOI from mutating left handside of assignment.
  clang::SourceLocation start_of_file = \
      src_mgr.getLocForStartOfFile(src_mgr.getMainFileID());
  range_lhs_assignment = new clang::SourceRange(
        start_of_file, start_of_file);

  // Initialize set of mutation operators to be applied
  vector<Mutators> selected_mutators = config->getSelectedMutators();
  for (unsigned i = 0; i != selected_mutators.size(); ++i) 
    switch (selected_mutators[i]) {
      case aor:
        mutation_operators.push_back(new AOR());
        break;
      case bor:
        mutation_operators.push_back(new BOR());
        break;  
      case lcr:
        mutation_operators.push_back(new LCR());
        break;
      case sbr:
        mutation_operators.push_back(new SBR());
        break;
      case sor:
        mutation_operators.push_back(new SOR());
        break;
      case ror:
        mutation_operators.push_back(new ROR());
        break;
      case uoi:
        mutation_operators.push_back(new UOI());
        break;
      default:
        cerr << "Unknown mutation operators: " << selected_mutators[i] << endl;
        exit(1);      
    }
}

void LemonASTVisitor::checkStmt(clang::Stmt *s) {
  if (isInLhsAssignment) {
    // If ASTVisitor was in LHS assignment,
    // need to constantly check if it is still in. Update when get out of LHS.
    SourceLocation start_loc = getPreciseLocation(s->getBeginLoc(), CI, true);
    if (range_lhs_assignment->getEnd() < start_loc)
      isInLhsAssignment = false;
  } 

  if (!isInLhsAssignment) {
    // If ASTVisitor was not in LHS assignment,
    // check if it is entering one.
    BinaryOperator *bo;
    if ((bo = dyn_cast<BinaryOperator>(s))) {
      if (bo->isAssignmentOp()) {
        delete range_lhs_assignment;
        SourceLocation start_loc = getPreciseLocation(s->getBeginLoc(), CI, true);
        range_lhs_assignment = new SourceRange(start_loc, bo->getOperatorLoc());
        isInLhsAssignment = true;
      }
    }
  }
}

bool LemonASTVisitor::VisitStmt(clang::Stmt *s) {
  checkStmt(s);

  for (auto mutation_operator: mutation_operators) {
    if (isInLhsAssignment && mutation_operator->getName().compare("UOI") == 0)
      continue;

    if (mutation_operator->canMutate(s, config, CI))
      mutation_operator->mutate(s, database);
  }

  return true;
}
