#ifndef UOR_H_
#define UOR_H_

#include "template.h"

class UOR : public MutationOperatorTemplate {
public:
  UOR(std::string name = "UOR", 
      std::string description = "Unary Operator Replacement") 
      : MutationOperatorTemplate(name, description) {}

  virtual bool canMutate(
      clang::Stmt *s, Configuration *config, clang::CompilerInstance &comp_inst);
  virtual void mutate(clang::Stmt *s, MutantDatabase *database);

private:
  // void mutateOperator(
  //     clang::BinaryOperator *bo, MutantDatabase *database, int target_line);
  // void mutateWholeExpr(
  //     clang::BinaryOperator *bo, MutantDatabase *database, int target_line);
};

#endif  // UOR_H_
