#ifndef UOI_H_
#define UOI_H_

#include "clang/AST/Expr.h"

#include "template.h"

class UOI : public MutationOperatorTemplate {
public:
  UOI(std::string name = "UOI", 
      std::string description = "Unary Operator Insertion") 
      : MutationOperatorTemplate(name, description) {}

  virtual bool canMutate(
      clang::Stmt *s, Configuration *config, clang::CompilerInstance &comp_inst);
  virtual void mutate(clang::Stmt *s, MutantDatabase *database);
};

#endif  // UOI_H_
