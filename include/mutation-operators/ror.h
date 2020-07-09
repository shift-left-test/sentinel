#ifndef ROR_H_
#define ROR_H_

#include "template.h"

class ROR : public MutationOperatorTemplate {
public:
  ROR(std::string name = "ROR", 
      std::string description = "Relational Operator Replacement") 
      : MutationOperatorTemplate(name, description) {}

  virtual bool canMutate(
      clang::Stmt *s, Configuration *config, clang::CompilerInstance &comp_inst);
  virtual void mutate(clang::Stmt *s, MutantDatabase *database);
  
private:
  void mutateOperator(
      clang::BinaryOperator *bo, MutantDatabase *database, int target_line);
  void mutateWholeExpr(
      clang::BinaryOperator *bo, MutantDatabase *database, int target_line);
};

#endif  // ROR_H_
