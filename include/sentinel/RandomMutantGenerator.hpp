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
#include <filesystem>  // NOLINT
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "sentinel/MutantGenerator.hpp"
#include "sentinel/Mutants.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/operators/MutationOperator.hpp"
#include "sentinel/operators/aor.hpp"
#include "sentinel/operators/bor.hpp"
#include "sentinel/operators/lcr.hpp"
#include "sentinel/operators/ror.hpp"
#include "sentinel/operators/sdl.hpp"
#include "sentinel/operators/sor.hpp"
#include "sentinel/operators/uoi.hpp"

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
  explicit RandomMutantGenerator(const std::filesystem::path& path);

  Mutants populate(const SourceLines& sourceLines, std::size_t maxMutants, unsigned randomSeed) override;

 private:
  std::filesystem::path mDbPath;

  /**
   * @brief SentinelASTVistor class
   *        defines what to do at each type of AST node.
   */
  class SentinelASTVisitor : public clang::RecursiveASTVisitor<SentinelASTVisitor> {
   public:
    /**
     * @brief Default constructor
     *
     * @param context Clang AST context
     * @param mutables list of generated mutables
     * @param targetLines list of target line numbers
     * @param selectedOps list of operator names to use (empty means all)
     */
    SentinelASTVisitor(clang::ASTContext* context, Mutants* mutables, const std::vector<std::size_t>& targetLines,
                       const std::vector<std::string>& selectedOps);

    /**
     * @brief Default destructor
     */
    virtual ~SentinelASTVisitor();

    bool VisitStmt(clang::Stmt* s);

   private:
    clang::ASTContext* mContext;
    clang::SourceManager& mSrcMgr;
    std::vector<std::unique_ptr<MutationOperator>> mMutationOperators;
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
     * @param ci Clang compiler instance
     * @param mutables list of generated mutables
     * @param targetLines list of target line numbers
     * @param selectedOps list of operator names to use (empty means all)
     */
    SentinelASTConsumer(const clang::CompilerInstance& ci, Mutants* mutables,
                        const std::vector<std::size_t>& targetLines, const std::vector<std::string>& selectedOps);

    /**
     * @brief A callback function triggered when ASTs for translation unit
     *        has been parsed and ready for traversal.
     *
     * @param context Clang object holding long-lived AST nodes.
     */
    void HandleTranslationUnit(clang::ASTContext& context) override;

   private:
    // SentinelASTVisitor mVisitor;
    Mutants* mMutants;
    std::vector<std::size_t> mTargetLines;
    std::vector<std::string> mSelectedOps;
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
     * @param targetLines list of target line numbers
     * @param selectedOps list of operator names to use (empty means all)
     */
    GenerateMutantAction(Mutants* mutables, const std::vector<std::size_t>& targetLines,
                         const std::vector<std::string>& selectedOps);

    /**
     * @brief Create an ASTConsumer object to identify mutation locations
     *        in the AST of target file.
     *
     * @param ci Clang compiler management object
     * @param inFile target file
     */
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& ci, llvm::StringRef inFile) override;

   protected:
    void ExecuteAction() override;

   private:
    Mutants* mMutants;
    std::vector<std::size_t> mTargetLines;
    std::vector<std::string> mSelectedOps;
  };

  /**
   * @brief Returns a new FrontendActionFactory for GenerateMutantAction
   *
   * @param mutables list of generated mutables
   * @param targetLines list of target line numbers
   * @param selectedOps list of operator names to use (empty means all)
   */
  std::unique_ptr<clang::tooling::FrontendActionFactory> myNewFrontendActionFactory(
      Mutants* mutables, const std::vector<std::size_t>& targetLines, const std::vector<std::string>& selectedOps);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_RANDOMMUTANTGENERATOR_HPP_
