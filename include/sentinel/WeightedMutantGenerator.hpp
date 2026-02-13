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
#include <iostream>
#include <map>
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
#include "sentinel/operators/sdl.hpp"
#include "sentinel/operators/sor.hpp"
#include "sentinel/operators/uoi.hpp"
#include "sentinel/SourceLines.hpp"

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
  explicit WeightedMutantGenerator(const std::string& path);

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
    SentinelASTVisitor(clang::ASTContext* Context,
                       Mutants* mutables,
                       const SourceLines& targetLines,
                       DepthMap* depthMap);

    /**
     * @brief Default destructor
     */
    virtual ~SentinelASTVisitor();

    /**
     * @brief Callback function each time the visitor meets a Stmt node.
     */
    bool VisitStmt(clang::Stmt *s);

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
    std::vector<MutationOperator*> mMutationOperators;
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
     * @param CI Clang compiler management object
     */
    SentinelASTConsumer(const clang::CompilerInstance& CI, Mutants* mutables,
                        const SourceLines& targetLines, DepthMap* depthMap);

    /**
     * @brief A callback function triggered when ASTs for translation unit
     *        has been parsed and ready for traversal.
     *
     * @param Context Clang object holding long-lived AST nodes.
     */
    void HandleTranslationUnit(clang::ASTContext &Context) override;

   private:
    // SentinelASTVisitor mVisitor;
    Mutants* mMutants;
    SourceLines mTargetLines;
    DepthMap* mDepthMap;
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
    GenerateMutantAction(Mutants* mutables, const SourceLines& targetLines, DepthMap* depthMap);

    /**
     * @brief Create an ASTConsumer object to identify mutation locations
     *        in the AST of target file.
     *
     * @param CI Clang compiler management object
     * @param InFile target file
     */
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI, llvm::StringRef InFile) override;

   protected:
    void ExecuteAction() override;

   private:
    Mutants* mMutants;
    SourceLines mTargetLines;
    DepthMap* mDepthMap;
  };

  /**
   * @brief Returns a new FrontendActionFactory for GenerateMutantAction
   *
   * @param mutables list of generated mutables
   */
  std::unique_ptr<clang::tooling::FrontendActionFactory>
  myNewFrontendActionFactory(Mutants* mutables, const SourceLines& targetLines, DepthMap* depthMap);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_WEIGHTEDMUTANTGENERATOR_HPP_
