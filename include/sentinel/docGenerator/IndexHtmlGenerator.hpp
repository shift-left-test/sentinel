/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_DOCGENERATOR_INDEXHTMLGENERATOR_HPP_
#define INCLUDE_SENTINEL_DOCGENERATOR_INDEXHTMLGENERATOR_HPP_

#include <cstddef>
#include <filesystem>  // NOLINT
#include <string>
#include <vector>
#include "sentinel/Config.hpp"
#include "sentinel/MutationSummary.hpp"
#include "sentinel/docGenerator/DocGenerator.hpp"

namespace sentinel {

/**
 * @brief IndexHtmlGenerator class
 */
class IndexHtmlGenerator : public DocGenerator {
 public:
  /**
   * @brief Constructor for directory (non-root) pages.
   *
   * @param root root index or not
   * @param dirName current dirName
   * @param sizeOfTargetFiles number of target files
   * @param score mutation score percentage
   * @param numerator killed mutants count
   * @param denominator total evaluated mutants count
   * @param skipped number of skipped mutants
   * @param skippedDetail breakdown of skipped types
   */
  IndexHtmlGenerator(bool root, const std::filesystem::path& dirName,
                     std::size_t sizeOfTargetFiles, unsigned int score,
                     std::size_t numerator, std::size_t denominator,
                     std::size_t skipped, const std::string& skippedDetail);

  /**
   * @brief Constructor for root page with full summary data.
   *
   * @param root root index or not
   * @param dirName current dirName
   * @param sizeOfTargetFiles number of target files
   * @param score mutation score percentage
   * @param numerator killed mutants count
   * @param denominator total evaluated mutants count (killed + survived)
   * @param summary aggregated mutation summary
   * @param config run configuration
   * @param timestamp report generation timestamp
   * @param version program version string
   */
  IndexHtmlGenerator(bool root, const std::filesystem::path& dirName,
                     std::size_t sizeOfTargetFiles, unsigned int score,
                     std::size_t numerator, std::size_t denominator,
                     const MutationSummary& summary, const Config& config,
                     const std::string& timestamp, const std::string& version);

  /**
   * @brief push a item to table
   *
   * @param subName dir name or file name
   * @param subScore dir's score or file's score
   * @param subNumerator
   * @param subDenominator
   * @param numOfFiles in dir (only used if root)
   */
  void pushItemToTable(const std::string& subName, int subScore,
                       std::size_t subNumerator, std::size_t subDenominator,
                       std::size_t numOfFiles);

  /**
   * @brief make html string
   */
  std::string str() const override;

  /**
   * @brief Format skipped mutant detail string.
   *
   * @param timeout    Number of timeout mutants.
   * @param buildFailure Number of build failure mutants.
   * @param runtimeError Number of runtime error mutants.
   * @return Formatted string (e.g., "3 timeout · 2 build failure").
   */
  static std::string formatSkippedDetail(std::size_t timeout,
                                         std::size_t buildFailure,
                                         std::size_t runtimeError);

 private:
  std::string buildCardsHtml() const;
  std::string buildRootPanelsHtml() const;
  std::string buildConfigHtml() const;
  std::string buildTableHtml() const;
  std::string buildBreadcrumbHtml() const;

  static std::string formatDuration(double secs);
  static const char* coverageClass(unsigned int cov);
  static const char* coverageFillClass(unsigned int cov);

  bool mRoot;
  std::filesystem::path mDirName;
  std::size_t mSizeOfTargetFiles;
  unsigned int mScore;
  std::size_t mNumerator;
  std::size_t mDenominator;
  std::size_t mSkipped = 0;
  std::string mSkippedDetail;
  std::string mTableItem;

  const MutationSummary* mSummary = nullptr;
  const Config* mConfig = nullptr;
  std::string mTimestamp;
  std::string mVersion;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_DOCGENERATOR_INDEXHTMLGENERATOR_HPP_
