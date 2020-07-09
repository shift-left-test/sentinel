#ifndef AOR_H_
#define AOR_H_

#include "template.h"

class AOR : public MutationOperatorTemplate {
public:
  AOR(std::string name = "AOR", 
      std::string description = "Arithmetic Operator Replacement") 
      : MutationOperatorTemplate(name, description) {}

  virtual bool canMutate(
      clang::Stmt *s, Configuration *config, clang::CompilerInstance &comp_inst);
  virtual void mutate(clang::Stmt *s, MutantDatabase *database);

private:
  void mutateOperator(
      clang::BinaryOperator *bo, MutantDatabase *database, int target_line);
  void mutateWholeExpr(
      clang::BinaryOperator *bo, MutantDatabase *database, int target_line);
  bool violateCSyntax(
      clang::BinaryOperator *bo, std::string mutated_operator, 
      clang::CompilerInstance &comp_inst);
};

#endif  // AOR_H_
