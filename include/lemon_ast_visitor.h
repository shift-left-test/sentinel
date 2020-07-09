#ifndef LEMON_AST_VISITOR_H_
#define LEMON_AST_VISITOR_H_ 

#include <vector>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Basic/SourceLocation.h"

#include "mutant_database.h"
#include "template.h"

class LemonASTVisitor : public clang::RecursiveASTVisitor<LemonASTVisitor>
{
private:
  clang::CompilerInstance &CI;
  clang::SourceManager &src_mgr;
  MutantDatabase *database;
  Configuration *config;
  std::vector<MutationOperatorTemplate*> mutation_operators;

  clang::SourceRange *range_lhs_assignment;
  bool isInLhsAssignment;

  void checkStmt(clang::Stmt *s);

public:
  LemonASTVisitor(
      clang::CompilerInstance &comp_inst, MutantDatabase *database);

  bool VisitStmt(clang::Stmt *s);
};

#endif    // LEMON_AST_VISITOR_H_