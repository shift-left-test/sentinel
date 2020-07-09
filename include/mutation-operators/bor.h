#ifndef BOR_H_
#define BOR_H_

#include "template.h"

class BOR : public MutationOperatorTemplate {
public:
  BOR(std::string name = "BOR", 
      std::string description = "Bitwise Operator Replacement") 
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

#endif  // BOR_H_
