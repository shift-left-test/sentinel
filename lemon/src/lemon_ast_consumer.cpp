#include "lemon_ast_consumer.h"

LemonASTConsumer::LemonASTConsumer(
    clang::CompilerInstance &comp_inst, MutantDatabase *database) 
  : Visitor(comp_inst, database) {}

void LemonASTConsumer::HandleTranslationUnit(clang::ASTContext &Context)
{
  /* we can use ASTContext to get the TranslationUnitDecl, which is
  a single Decl that collectively represents the entire source file */
  Visitor.TraverseDecl(Context.getTranslationUnitDecl());
}