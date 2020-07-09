#ifndef SBR_H_
#define SBR_H_

#include "clang/AST/Expr.h"

#include "template.h"

class SBR : public MutationOperatorTemplate {
public:
  SBR(std::string name = "SBR", 
      std::string description = "Statement Break Replacement / Statement Deletion") 
      : MutationOperatorTemplate(name, description) {}

  virtual bool canMutate(
      clang::Stmt *s, Configuration *config, clang::CompilerInstance &comp_inst);
  virtual void mutate(clang::Stmt *s, MutantDatabase *database);

private:
  const clang::Stmt* getImmediateParentCompoundStmt(
      const clang::Stmt *stmt_ptr, clang::CompilerInstance &comp_inst);
  bool isCompleteStmt(clang::Stmt *s, clang::CompilerInstance &comp_inst);
  bool isLastStmtOfStmtExpr(
      clang::Stmt *s, const clang::CompoundStmt *cs, 
      clang::CompilerInstance &comp_inst);

};

#endif  // SBR_H_
