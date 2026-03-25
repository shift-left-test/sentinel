/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_WEIGHTEDMUTANTGENERATOR_HPP_
#define INCLUDE_SENTINEL_WEIGHTEDMUTANTGENERATOR_HPP_

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include <filesystem>  // NOLINT
#include <iostream>
#include <map>
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
 * @brief DepthMap type definition
 */
using DepthMap = std::map<SourceLine, int>;

/**
 * @brief WeightedMutantGenerator class
 */
class WeightedMutantGenerator : public MutantGenerator {
 public:
  /**
   * @brief Default constructor
   *
   * @param path to compilation database file
   */
  explicit WeightedMutantGenerator(const std::filesystem::path& path);

  Mutants generate(const SourceLines& sourceLines, std::size_t maxMutants, unsigned int randomSeed) override;

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
     * @param depthMap map from source line to AST depth
     * @param selectedOps list of operator names to use (empty means all)
     */
    SentinelASTVisitor(clang::ASTContext* context, Mutants* mutables, const SourceLines& targetLines,
                       DepthMap* depthMap, const std::vector<std::string>& selectedOps);

    /**
     * @brief Default destructor
     */
    virtual ~SentinelASTVisitor();

    /**
     * @brief Callback function each time the visitor meets a Stmt node.
     */
    bool VisitStmt(clang::Stmt* s);

    /**
     * @brief Return the depth of a Stmt AST node.
     *        Depth is measured through upward traversal from target AST node.
     *        Depth is increased when a CompoundStmt ({...}: curly braces) node
     *        is encountered.
     *        Traversal stops when FunctionDecl node is encountered, or
     *        cannot go upward anymore (target node is not inside a function).
     *
     * @param s target node
     * @return depth of s
     */
    int getDepth(clang::Stmt* s);

   private:
    clang::ASTContext* mContext;
    clang::SourceManager& mSrcMgr;
    std::vector<std::unique_ptr<MutationOperator>> mMutationOperators;
    Mutants* mMutants;
    SourceLines mTargetLines;
    DepthMap* mDepthMap;
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
     * @param depthMap map from source line to AST depth
     * @param selectedOps list of operator names to use (empty means all)
     */
    SentinelASTConsumer(const clang::CompilerInstance& ci, Mutants* mutables, const SourceLines& targetLines,
                        DepthMap* depthMap, const std::vector<std::string>& selectedOps);

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
    SourceLines mTargetLines;
    DepthMap* mDepthMap;
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
     * @param depthMap map from source line to AST depth
     * @param selectedOps list of operator names to use (empty means all)
     */
    GenerateMutantAction(Mutants* mutables, const SourceLines& targetLines, DepthMap* depthMap,
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
    SourceLines mTargetLines;
    DepthMap* mDepthMap;
    std::vector<std::string> mSelectedOps;
  };

  /**
   * @brief Returns a new FrontendActionFactory for GenerateMutantAction
   *
   * @param mutables list of generated mutables
   * @param targetLines list of target line numbers
   * @param depthMap map from source line to AST depth
   * @param selectedOps list of operator names to use (empty means all)
   */
  std::unique_ptr<clang::tooling::FrontendActionFactory> myNewFrontendActionFactory(
      Mutants* mutables, const SourceLines& targetLines, DepthMap* depthMap,
      const std::vector<std::string>& selectedOps);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_WEIGHTEDMUTANTGENERATOR_HPP_
