/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_RANDOMMUTANTGENERATOR_HPP_
#define INCLUDE_SENTINEL_RANDOMMUTANTGENERATOR_HPP_

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "sentinel/Mutants.hpp"
#include "sentinel/MutantGenerator.hpp"
#include "sentinel/operators/MutationOperator.hpp"
#include "sentinel/operators/aor.hpp"
#include "sentinel/operators/bor.hpp"
#include "sentinel/operators/lcr.hpp"
#include "sentinel/operators/ror.hpp"
#include "sentinel/operators/sor.hpp"
#include "sentinel/operators/sdl.hpp"
#include "sentinel/operators/uoi.hpp"
#include "sentinel/SourceLines.hpp"

namespace sentinel {

/**
 * @brief RandomMutantGenerator class
 */
class RandomMutantGenerator : public MutantGenerator {
 public:
  /**
   * @brief Default constructor
   *
   * @param path to compilation database file
   */
  explicit RandomMutantGenerator(const std::string& path) : mDbPath(path) {
  }

  Mutants populate(const SourceLines& sourceLines, std::size_t maxMutants) override;

  Mutants populate(const SourceLines& sourceLines, std::size_t maxMutants, unsigned randomSeed) override;

 private:
  std::string mDbPath;

  /**
   * @brief SentinelASTVistor class
   *        defines what to do at each type of AST node.
   */
  class SentinelASTVisitor : public clang::RecursiveASTVisitor<SentinelASTVisitor> {
   public:
    /**
     * @brief Default constructor
     *
     * @param CI Clang compiler management object
     * @param mutables list of generated mutables
     * @param targetLines list of target line numbers
     */
    SentinelASTVisitor(clang::ASTContext* Context, Mutants* mutables,
                       const std::vector<std::size_t>& targetLines);

    /**
     * @brief Default destructor
     */
    virtual ~SentinelASTVisitor();

    bool VisitStmt(clang::Stmt *s);

   private:
    clang::ASTContext* mContext;
    clang::SourceManager& mSrcMgr;
    std::vector<MutationOperator*> mMutationOperators;
    Mutants* mMutants;
    std::vector<std::size_t> mTargetLines;
  };

  /**
   * @brief SentinelASTConsumer class
   */
  class SentinelASTConsumer : public clang::ASTConsumer {
   public:
    /**
     * @brief Default constructor
     *
     * @param CI Clang compiler management object
     */
    SentinelASTConsumer(const clang::CompilerInstance& CI,
                        Mutants* mutables,
                        const std::vector<std::size_t>& targetLines) :
        mMutants(mutables), mTargetLines(targetLines) {
    }

    /**
     * @brief A callback function triggered when ASTs for translation unit
     *        has been parsed and ready for traversal.
     *
     * @param Context Clang object holding long-lived AST nodes.
     */
    void HandleTranslationUnit(clang::ASTContext &Context) override {
      SentinelASTVisitor mVisitor(&Context, mMutants, mTargetLines);
      mVisitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

   private:
    // SentinelASTVisitor mVisitor;
    Mutants* mMutants;
    std::vector<std::size_t> mTargetLines;
  };

  /**
   * @brief GenerateMutantAction class
   */
  class GenerateMutantAction : public clang::ASTFrontendAction {
   public:
    /**
     * @brief Default constructor
     *
     * @param mutables list of generated mutables (output)
     * @param mTargetLines list of target line numbers
     */
    GenerateMutantAction(Mutants* mutables, const std::vector<std::size_t>& targetLines) :
        mMutants(mutables), mTargetLines(targetLines) {
    }

    /**
     * @brief Create an ASTConsumer object to identify mutation locations
     *        in the AST of target file.
     *
     * @param CI Clang compiler management object
     * @param InFile target file
     */
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI,
                                                          llvm::StringRef InFile) override {
      CI.getDiagnostics().setClient(new clang::IgnoringDiagConsumer());
      return std::unique_ptr<clang::ASTConsumer>(new SentinelASTConsumer(CI, mMutants, mTargetLines));
    }

   protected:
    void ExecuteAction() override;

   private:
    Mutants* mMutants;
    std::vector<std::size_t> mTargetLines;
  };

  /**
   * @brief Returns a new FrontendActionFactory for GenerateMutantAction
   *
   * @param mutables list of generated mutables
   */
  std::unique_ptr<clang::tooling::FrontendActionFactory>
  myNewFrontendActionFactory(Mutants* mutables, const std::vector<std::size_t>& targetLines);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_RANDOMMUTANTGENERATOR_HPP_
