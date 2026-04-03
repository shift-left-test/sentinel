/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_HTMLREPORT_HPP_
#define INCLUDE_SENTINEL_HTMLREPORT_HPP_

#include <filesystem>  // NOLINT
#include <string>
#include <vector>
#include "sentinel/Config.hpp"
#include "sentinel/Report.hpp"

namespace sentinel {

/**
 * @brief HtmlReport class
 */
class HtmlReport : public Report {
 public:
  /**
   * @brief Construct an HTML report view from a precomputed summary.
   *
   * @param summary Aggregated mutation data to render.
   */
  explicit HtmlReport(const MutationSummary& summary);

  /**
   * @brief Construct an HTML report view from a precomputed summary and config.
   *
   * @param summary Aggregated mutation data to render.
   * @param config  Run configuration used to generate the report.
   */
  HtmlReport(const MutationSummary& summary, const Config& config);

  /**
   * @brief Destructor.
   */
  ~HtmlReport() override;

  /**
   * @brief Save the report in HTML format to @p dirPath.
   *
   * @param dirPath Directory to save the HTML files into.
   * @throw InvalidArgumentException when path is not a directory.
   */
  void save(const std::filesystem::path& dirPath) override;

 private:
  /**
   * @brief Initialize timestamp and version fields.
   */
  void initMetadata();

  /**
   * @brief makeIndexHtml
   *
   * @param totNumberOfMutation
   * @param totNumberOfDetectedMutation
   * @param root index or not
   * @param currentDirPath current key of groupByDirPath (used in non root)
   * @param outputDir
   */
  void makeIndexHtml(std::size_t totNumberOfMutation, std::size_t totNumberOfDetectedMutation, bool root,
                     const std::filesystem::path& currentDirPath, const std::filesystem::path& outputDir);

  /**
   * @brief makeSourceHtml
   *
   * @param mrs Mutation Results of a source file
   * @param srcPath
   * @param outputDir
   */
  void makeSourceHtml(const std::vector<const MutationResult*>& mrs, const std::filesystem::path& srcPath,
                      const std::filesystem::path& outputDir);

  Config mConfig;             ///< Run configuration
  std::string mTimestamp;     ///< Report generation timestamp
  std::string mVersion;       ///< Program version
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_HTMLREPORT_HPP_
