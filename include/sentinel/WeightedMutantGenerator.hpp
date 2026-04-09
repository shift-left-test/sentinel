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
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/MutantGenerator.hpp"
#include "sentinel/SourceLines.hpp"

namespace sentinel {

/**
 * @brief DepthMap type definition
 */
using DepthMap = std::map<SourceLine, int>;

/**
 * @brief Weighted mutant generator — prioritizes deeper AST nodes.
 *
 * Overrides collectAllMutants() to use its own AST visitor that tracks
 * DepthMap alongside mutant collection.
 */
class WeightedMutantGenerator : public MutantGenerator {
 public:
  /**
   * @brief Default constructor
   *
   * @param path to compilation database file
   */
  explicit WeightedMutantGenerator(const std::filesystem::path& path);

 protected:
  Mutants collectAllMutants(const SourceLines& sourceLines) override;

  Mutants selectMutants(const SourceLines& sourceLines, std::size_t maxMutants,
                        unsigned int randomSeed, const CandidateIndex& index,
                        std::size_t mutantsPerLine) override;

 private:
  DepthMap mDepthMap;

  /**
   * @brief Extended AST visitor that also computes AST depth for each target line.
   */
  class DepthAwareASTVisitor : public clang::RecursiveASTVisitor<DepthAwareASTVisitor> {
   public:
    /**
     * @brief Construct a depth-aware visitor for collecting mutants and AST depth.
     *
     * @param context Clang AST context
     * @param mutables list of generated mutables
     * @param targetLines list of target source lines
     * @param depthMap map from source line to AST depth
     * @param selectedOps list of operator names to use (empty means all)
     */
    DepthAwareASTVisitor(clang::ASTContext* context, Mutants* mutables,
                         const SourceLines& targetLines, DepthMap* depthMap,
                         const std::vector<std::string>& selectedOps);

    ~DepthAwareASTVisitor();

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
   * @brief Depth-aware AST consumer.
   */
  class DepthAwareASTConsumer : public clang::ASTConsumer {
   public:
    /**
     * @brief Construct a depth-aware AST consumer.
     *
     * @param ci Clang compiler instance
     * @param mutables list of generated mutables
     * @param targetLines list of target source lines
     * @param depthMap map from source line to AST depth
     * @param selectedOps list of operator names to use (empty means all)
     */
    DepthAwareASTConsumer(const clang::CompilerInstance& ci, Mutants* mutables,
                          const SourceLines& targetLines, DepthMap* depthMap,
                          const std::vector<std::string>& selectedOps);

    void HandleTranslationUnit(clang::ASTContext& context) override;

   private:
    Mutants* mMutants;
    SourceLines mTargetLines;
    DepthMap* mDepthMap;
    std::vector<std::string> mSelectedOps;
  };

  /**
   * @brief Depth-aware Clang frontend action.
   */
  class DepthAwareAction : public clang::ASTFrontendAction {
   public:
    /**
     * @brief Construct a depth-aware frontend action.
     *
     * @param mutables list of generated mutables (output)
     * @param targetLines list of target source lines
     * @param depthMap map from source line to AST depth
     * @param selectedOps list of operator names to use (empty means all)
     */
    DepthAwareAction(Mutants* mutables, const SourceLines& targetLines,
                     DepthMap* depthMap, const std::vector<std::string>& selectedOps);

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance& ci, llvm::StringRef inFile) override;

   private:
    Mutants* mMutants;
    SourceLines mTargetLines;
    DepthMap* mDepthMap;
    std::vector<std::string> mSelectedOps;
  };

  /**
   * @brief Returns a new FrontendActionFactory for DepthAwareAction
   *
   * @param mutables list of generated mutables
   * @param targetLines list of target source lines
   * @param depthMap map from source line to AST depth
   * @param selectedOps list of operator names to use (empty means all)
   */
  std::unique_ptr<clang::tooling::FrontendActionFactory> createDepthAwareActionFactory(
      Mutants* mutables, const SourceLines& targetLines, DepthMap* depthMap,
      const std::vector<std::string>& selectedOps);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_WEIGHTEDMUTANTGENERATOR_HPP_
