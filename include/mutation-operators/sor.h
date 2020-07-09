#ifndef SOR_H_
#define SOR_H_

#include "template.h"

class SOR : public MutationOperatorTemplate {
public:
  SOR(std::string name = "SOR", 
      std::string description = "Shift Operator Replacement") 
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

#endif  // SOR_H_
