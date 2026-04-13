/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_HTMLREPORT_HPP_
#define INCLUDE_SENTINEL_HTMLREPORT_HPP_

#include <filesystem>  // NOLINT
#include <string>
#include "sentinel/Config.hpp"
#include "sentinel/Report.hpp"

namespace sentinel {

/**
 * @brief HtmlReport class
 *
 * Generates a single self-contained HTML file with embedded CSS and JavaScript
 * that implements a SPA (Single-Page Application) for mutation testing reports.
 */
class HtmlReport : public Report {
 public:
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
   * @brief Save the report as a single index.html to @p dirPath.
   *
   * @param dirPath Directory to save the HTML file into.
   * @throw InvalidArgumentException when path is not a directory.
   */
  void save(const std::filesystem::path& dirPath) override;

 private:
  /**
   * @brief Initialize timestamp and version fields.
   */
  void initMetadata();

  /**
   * @brief Build the JSON data object containing all mutation report data.
   *
   * @return JSON string with version, timestamp, config, summary, dirs, files.
   */
  std::string buildJsonData() const;

  /**
   * @brief Build the JSON representation of the run configuration.
   *
   * @return JSON object string with config fields.
   */
  std::string buildConfigJson() const;

  /**
   * @brief Escape a string for safe embedding in JSON.
   *
   * @param s Input string.
   * @return JSON-escaped string.
   */
  static std::string jsonEscape(const std::string& s);

  Config mConfig;             ///< Run configuration
  std::string mTimestamp;     ///< Report generation timestamp
  std::string mVersion;       ///< Program version
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_HTMLREPORT_HPP_
