/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTANTGENERATOR_HPP_
#define INCLUDE_SENTINEL_MUTANTGENERATOR_HPP_

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>
#include "sentinel/Config.hpp"
#include "sentinel/Mutants.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/operators/MutationOperator.hpp"

namespace sentinel {

/**
 * @brief Pre-built index of all mutant candidates grouped by file.
 *
 * Move-only: mutantsByFile stores raw pointers into allMutants,
 * so copying would create dangling pointers.
 */
struct CandidateIndex {
  /**
   * @brief Default constructor
   */
  CandidateIndex() = default;

  /**
   * @brief Move constructor
   */
  CandidateIndex(CandidateIndex&&) = default;

  /**
   * @brief Move assignment operator
   */
  CandidateIndex& operator=(CandidateIndex&&) = default;

  CandidateIndex(const CandidateIndex&) = delete;
  CandidateIndex& operator=(const CandidateIndex&) = delete;

  /**
   * @brief All collected mutant candidates.
   */
  Mutants allMutants;

  /**
   * @brief Mutant pointers grouped by canonical file path, sorted by line.
   */
  std::map<std::filesystem::path, std::vector<const Mutant*>> mutantsByFile;
};

/**
 * @brief MutantGenerator base class with Template Method pattern.
 *
 * The generate() method orchestrates the full pipeline:
 * 1. collectAllMutants() — AST traversal to find mutation candidates
 * 2. buildCandidateIndex() — group mutants by file for efficient lookup
 * 3. selectMutants() — subclass-specific selection strategy
 */
class MutantGenerator {
 public:
  virtual ~MutantGenerator();

  /**
   * @brief Template Method: orchestrates mutant generation pipeline.
   */
  Mutants generate(const SourceLines& sourceLines, std::size_t maxMutants, unsigned int randomSeed,
                   std::size_t mutantsPerLine = 1);

  /**
   * @brief Set mutation operators to use. If empty, all operators are used.
   *
   * @param ops list of operator names (e.g. "AOR", "BOR")
   */
  void setOperators(const std::vector<std::string>& ops) {
    mSelectedOperators = ops;
  }

  /**
   * @brief Return a new mutant generator instance based on the specified options
   *
   * @param generator name
   * @param directory path to the directory containing the compile_commands.json file
   * @return mutant generator instance
   */
  static std::shared_ptr<MutantGenerator> getInstance(Generator generator,
                                                      const std::filesystem::path& directory);

  /**
   * @brief Return the total number of candidate mutants found before the limit was applied.
   *
   * @return candidate count from the last generate() call
   */
  std::size_t getCandidateCount() const {
    return mCandidateCount;
  }

  /**
   * @brief Return the number of mutable lines per file from the last generate() call.
   *
   * @return map of canonical file path to mutable line count
   */
  const std::map<std::filesystem::path, std::size_t>& getLinesByPath() const {
    return mLinesByPath;
  }

 protected:
  /**
   * @brief Constructor with compilation database path.
   *
   * @param dbPath path to the directory containing compile_commands.json
   */
  explicit MutantGenerator(const std::filesystem::path& dbPath) : mDbPath(dbPath) {}

  /**
   * @brief Collect all mutant candidates via Clang AST traversal.
   *        Override to extend AST visitor behavior (e.g., depth tracking).
   *
   * @param sourceLines list of target source lines
   * @return all mutant candidates
   */
  virtual Mutants collectAllMutants(const SourceLines& sourceLines);

  /**
   * @brief Build index from collected mutants for efficient lookup.
   *
   * @param mutants collected mutant candidates
   * @return candidate index
   */
  CandidateIndex buildCandidateIndex(Mutants mutants);

  /**
   * @brief Subclass-specific mutant selection strategy.
   *
   * @param sourceLines list of target source lines
   * @param maxMutants limit number of generated mutables
   * @param randomSeed random seed
   * @param index pre-built candidate index
   * @param mutantsPerLine number of mutants to select per source line
   * @return selected mutants
   */
  virtual Mutants selectMutants(const SourceLines& sourceLines, std::size_t maxMutants,
                                unsigned int randomSeed, const CandidateIndex& index,
                                std::size_t mutantsPerLine) = 0;

  /**
   * @brief Find candidate mutants covering a specific line in a file.
   *
   * @param index candidate index
   * @param canonPath canonical file path
   * @param targetLine target line number
   * @param out output vector (cleared and populated with results)
   */
  static void findCandidatesForLine(const CandidateIndex& index, const std::filesystem::path& canonPath,
                                    std::size_t targetLine, std::vector<const Mutant*>* out);

  /**
   * @brief Select up to maxPerLine unique mutants from shuffled candidates.
   *
   * Shuffles the candidate list, finds mutants not already in selectedSet,
   * inserts them, and appends them to result.
   *
   * @param candidates candidate mutant pointers (shuffled in-place)
   * @param rng random number generator
   * @param selectedSet set of already-selected mutants
   * @param result output vector to append selected mutants
   * @param maxPerLine maximum mutants to select per line (0 = unlimited)
   */
  static void selectUniqueCandidates(std::vector<const Mutant*>* candidates,
                                     std::mt19937* rng,
                                     std::set<Mutant>* selectedSet,
                                     Mutants* result,
                                     std::size_t maxPerLine) {
    std::shuffle(candidates->begin(), candidates->end(), *rng);
    std::size_t count = 0;
    for (const auto* candidate : *candidates) {
      if (maxPerLine > 0 && count >= maxPerLine) {
        break;
      }
      // cppcheck-suppress useStlAlgorithm
      if (selectedSet->insert(*candidate).second) {
        result->push_back(*candidate);
        ++count;
      }
    }
  }

  /**
   * @brief Select mutants per source line by iterating over a range.
   *
   * Shared logic for Uniform and Weighted generators. The LineExtractor
   * callable converts each element in the range to a const SourceLine&.
   *
   * @tparam Range iterable container type
   * @tparam LineExtractor callable: (const element&) -> const SourceLine&
   * @param range iteration source
   * @param maxMutants maximum number of mutants to select (0 = unlimited)
   * @param randomSeed seed for the random number generator
   * @param index pre-built candidate index
   * @param getLine extracts a SourceLine from each range element
   * @param mutantsPerLine number of mutants to select per source line
   * @return selected mutants
   */
  template <typename Range, typename LineExtractor>
  Mutants selectFromRange(const Range& range, std::size_t maxMutants,
                          unsigned int randomSeed, const CandidateIndex& index,
                          LineExtractor getLine, std::size_t mutantsPerLine) {
    std::set<Mutant> selectedSet;
    Mutants result;
    std::mt19937 rng(randomSeed);
    std::map<std::filesystem::path, std::filesystem::path> pathCache;
    std::size_t candidateLineCount = 0;
    std::vector<const Mutant*> candidates;

    for (const auto& elem : range) {
      const auto& line = getLine(elem);
      std::filesystem::path rawPath = line.getPath();
      auto emplaceResult = pathCache.emplace(rawPath, std::filesystem::path{});
      if (emplaceResult.second) {
        emplaceResult.first->second = std::filesystem::canonical(rawPath);
      }
      const std::filesystem::path& canonPath = emplaceResult.first->second;

      findCandidatesForLine(index, canonPath, line.getLineNumber(), &candidates);
      if (candidates.empty()) {
        continue;
      }

      candidateLineCount++;
      mLinesByPath[canonPath]++;

      if (maxMutants > 0 && result.size() >= maxMutants) {
        continue;
      }

      std::size_t effectivePerLine = mutantsPerLine;
      if (maxMutants > 0) {
        std::size_t remaining = maxMutants - result.size();
        if (effectivePerLine == 0 || effectivePerLine > remaining) {
          effectivePerLine = remaining;
        }
      }
      selectUniqueCandidates(&candidates, &rng, &selectedSet, &result, effectivePerLine);
    }

    mCandidateCount = candidateLineCount;
    return result;
  }

  /// @brief Path to the directory containing compile_commands.json.
  std::filesystem::path mDbPath;
  /// @brief Selected mutation operator names; empty means all operators.
  std::vector<std::string> mSelectedOperators;
  /// @brief Total candidate count from the last generate() call.
  std::size_t mCandidateCount = 0;
  /// @brief Mutable line count per file from the last generate() call.
  std::map<std::filesystem::path, std::size_t> mLinesByPath;

  /**
   * @brief SentinelASTVisitor — shared AST visitor for mutant collection.
   */
  class SentinelASTVisitor : public clang::RecursiveASTVisitor<SentinelASTVisitor> {
   public:
    /**
     * @brief Construct a visitor for collecting mutants on target lines.
     *
     * @param context Clang AST context
     * @param mutables list of generated mutables
     * @param targetLines list of target line numbers
     * @param selectedOps list of operator names to use (empty means all)
     */
    SentinelASTVisitor(clang::ASTContext* context, Mutants* mutables,
                       const std::vector<std::size_t>& targetLines,
                       const std::vector<std::string>& selectedOps);

    virtual ~SentinelASTVisitor();

    /**
     * @brief Visit a statement node and collect mutants on target lines.
     * @param s the statement node being visited
     * @return true to continue traversal
     */
    bool VisitStmt(clang::Stmt* s);

   private:
    clang::ASTContext* mContext;
    clang::SourceManager& mSrcMgr;
    std::vector<std::unique_ptr<MutationOperator>> mMutationOperators;
    Mutants* mMutants;
    std::vector<std::size_t> mTargetLines;

    void initOperators(clang::ASTContext* context, const std::vector<std::string>& selectedOps);
    bool isOnTargetLine(std::size_t startLineNum, std::size_t endLineNum) const;
    void populateMutants(clang::Stmt* s);
  };

  /**
   * @brief SentinelASTConsumer — shared AST consumer.
   */
  class SentinelASTConsumer : public clang::ASTConsumer {
   public:
    /**
     * @brief Construct an AST consumer that creates a SentinelASTVisitor.
     *
     * @param ci Clang compiler instance
     * @param mutables list of generated mutables
     * @param targetLines list of target line numbers
     * @param selectedOps list of operator names to use (empty means all)
     */
    SentinelASTConsumer(const clang::CompilerInstance& ci, Mutants* mutables,
                        const std::vector<std::size_t>& targetLines,
                        const std::vector<std::string>& selectedOps);

    /**
     * @brief Traverse the full AST after parsing is complete.
     * @param context the AST context for the translation unit
     */
    void HandleTranslationUnit(clang::ASTContext& context) override;

   private:
    Mutants* mMutants;
    std::vector<std::size_t> mTargetLines;
    std::vector<std::string> mSelectedOps;
  };

  /**
   * @brief GenerateMutantAction — shared Clang frontend action.
   */
  class GenerateMutantAction : public clang::ASTFrontendAction {
   public:
    /**
     * @brief Construct a frontend action for mutant generation.
     *
     * @param mutables list of generated mutables (output)
     * @param targetLines list of target line numbers
     * @param selectedOps list of operator names to use (empty means all)
     */
    GenerateMutantAction(Mutants* mutables, const std::vector<std::size_t>& targetLines,
                         const std::vector<std::string>& selectedOps);

    /**
     * @brief Create the AST consumer for the given source file.
     * @param ci the Clang compiler instance
     * @param inFile the input source file path
     * @return a new SentinelASTConsumer
     */
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance& ci, llvm::StringRef inFile) override;

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
  static std::unique_ptr<clang::tooling::FrontendActionFactory> createActionFactory(
      Mutants* mutables, const std::vector<std::size_t>& targetLines,
      const std::vector<std::string>& selectedOps);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTANTGENERATOR_HPP_
