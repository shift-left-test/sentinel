#ifndef LEMON_TEMPLATE_H_
#define LEMON_TEMPLATE_H_ 

#include <string>
#include <set>

#include "mutant_database.h"
#include "clang/AST/Stmt.h"
#include "clang/Frontend/CompilerInstance.h"

class MutationOperatorTemplate {
protected:
  std::string name;
  std::string description;
  std::set<std::string> domain;
  std::set<std::string> range;

public:
  MutationOperatorTemplate(std::string name, std::string description) 
      : name(name), description(description) {}

  std::string getName() { return name; }
  virtual bool canMutate(
      clang::Stmt *s, Configuration *config, 
      clang::CompilerInstance &comp_inst) = 0;
  virtual void mutate(clang::Stmt *s, MutantDatabase *database) = 0;
};

#endif  // LEMON_TEMPLATE_H_
