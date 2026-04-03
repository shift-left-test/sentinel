/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_DOCGENERATOR_SRCHTMLGENERATOR_HPP_
#define INCLUDE_SENTINEL_DOCGENERATOR_SRCHTMLGENERATOR_HPP_

#include <cstddef>
#include <filesystem>  // NOLINT
#include <string>
#include <tuple>
#include <vector>
#include "sentinel/docGenerator/DocGenerator.hpp"

namespace sentinel {

/**
 * @brief SrcHtmlGenerator class
 */
class SrcHtmlGenerator : public DocGenerator {
 public:
  /**
   * @brief Legacy constructor (backward compatibility)
   *
   * @param srcName current source file name
   * @param srcRoot if source file located at source root directory
   */
  explicit SrcHtmlGenerator(const std::string& srcName, bool srcRoot);

  /**
   * @brief New constructor with additional data for cards and breadcrumb
   *
   * @param srcName current source file name
   * @param srcRoot if source file located at source root directory
   * @param dirPath directory path of the source file
   * @param killed number of killed mutants
   * @param survived number of survived mutants
   * @param skipped number of skipped mutants
   * @param skippedDetail breakdown string for skipped types
   * @param version program version string
   */
  SrcHtmlGenerator(const std::string& srcName, bool srcRoot,
                   const std::filesystem::path& dirPath,
                   std::size_t killed, std::size_t survived, std::size_t skipped,
                   const std::string& skippedDetail, const std::string& version);

  /**
   * @brief push a source line
   *
   * @param curLineNum current line number
   * @param curClass killed or not or mixed
   * @param numCurLineMrs current line's number of mutation results
   * @param curCode current line's code
   * @param explain vector of tuple (count, operator, original, mutated, killed,
   *        killingTest)
   */
  void pushLine(std::size_t curLineNum, const std::string& curClass,
                std::size_t numCurLineMrs, const std::string& curCode,
                const std::vector<std::tuple<int, std::string, std::string,
                    std::string, bool, std::string>>& explain);

  /**
   * @brief push a mutation
   *
   * @param curLineNum current line number
   * @param killed or not
   * @param count mutation count
   * @param curKillingTest current killing test
   * @param curOperator current mutation operator
   */
  void pushMutation(std::size_t curLineNum, bool killed, std::size_t count,
                    const std::string& curKillingTest,
                    const std::string& curOperator);

  /**
   * @brief push a mutation operator
   *
   * @param mutator mutation operator name
   */
  void pushMutator(const std::string& mutator);

  /**
   * @brief push a killingTest
   *
   * @param killingTest killing test name
   */
  void pushKillingTest(const std::string& killingTest);

  /**
   * @brief make html string
   */
  std::string str() const override;

 private:
  /**
   * @brief escape reserved characters
   *
   * @param original string
   */
  std::string escape(const std::string& original) const;

  std::string buildCardsHtml() const;
  std::string buildBreadcrumbHtml() const;

  bool mSrcRoot;
  std::string mSrcName;
  std::string mLines;
  std::string mPopups;
  std::string mMutations;
  std::string mMutators;
  std::string mTestList;

  std::filesystem::path mDirPath;
  std::size_t mKilled = 0;
  std::size_t mSurvived = 0;
  std::size_t mSkipped = 0;
  std::string mSkippedDetail;
  std::string mVersion;
  bool mHasExtendedData = false;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_DOCGENERATOR_SRCHTMLGENERATOR_HPP_
