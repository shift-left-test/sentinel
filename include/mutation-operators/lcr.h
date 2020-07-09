#ifndef LCR_H_
#define LCR_H_

#include "template.h"

class LCR : public MutationOperatorTemplate {
public:
  LCR(std::string name = "LCR", 
      std::string description = "Logical Connector Replacement") 
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

#endif  // LCR_H_
