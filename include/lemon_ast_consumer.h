#ifndef LEMON_AST_CONSUMER_H_
#define LEMON_AST_CONSUMER_H_ 

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/Frontend/CompilerInstance.h"

#include "lemon_ast_visitor.h"
#include "mutant_database.h"

class LemonASTConsumer : public clang::ASTConsumer
{
public:
  LemonASTConsumer(
      clang::CompilerInstance &comp_inst, MutantDatabase *database);
  virtual void HandleTranslationUnit(clang::ASTContext &Context);

private:
  LemonASTVisitor Visitor;
};

#endif    // LEMON_AST_CONSUMER_H_  