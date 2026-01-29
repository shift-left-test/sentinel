/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_REPORT_HPP_
#define INCLUDE_SENTINEL_REPORT_HPP_

#include <experimental/filesystem>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include "sentinel/Logger.hpp"
#include "sentinel/MutationResults.hpp"

namespace sentinel {

/**
 * @brief Report interface
 */
class Report {
 public:
  /**
   * @brief Default Constructor
   *
   * @param results mutation results
   * @param sourcePath directory path of source files
   * @throw InvalidArgumentException
   *        when resultsPath is empty or sourcePath doesn't exist
   */
  Report(const MutationResults& results, const std::experimental::filesystem::path& sourcePath);

  /**
   * @brief Default Constructor
   *
   * @param resultsPath directory path of mutation results
   * @param sourcePath directory path of source files
   * @throw InvalidArgumentException
   *        when resultsPath is empty or sourcePath doesn't exist
   */
  Report(const std::experimental::filesystem::path& resultsPath,
         const std::experimental::filesystem::path& sourcePath);
  /**
   * @brief Default Destructor
   */
  virtual ~Report();
  /**
   * @brief Save the report to the given path
   *
   * @param path to save the report
   */
  virtual void save(const std::experimental::filesystem::path& path) = 0;

  /**
   * @brief Print summury of report
   */
  void printSummary();

 private:
  void generateReport();

 protected:
  /**
   * @brief Return the relative path from start dir
   *
   * @param path target path
   * @param start start dir
   * @return the relative path from start dir
   */
  std::experimental::filesystem::path getRelativePath(const std::string& path, const std::string& start);

  /**
   * @brief MutationResults instance
   */
  MutationResults mResults;

  /**
   * @brief group MutationReuslt by Directory
   */
  std::map<std::experimental::filesystem::path,
           std::tuple<std::vector<const MutationResult*>*, std::size_t,
                      std::size_t, std::size_t>* > groupByDirPath;

  /**
   * @brief group MutationResult by File
   */
  std::map<std::experimental::filesystem::path,
           std::tuple<std::vector<const MutationResult*>*,
                      std::size_t, std::size_t>* > groupByPath;

  /**
   * @brief total Number Of Mutation
   */
  std::size_t totNumberOfMutation = 0;

  /**
   * @brief total number of Deteced Mutation
   */
  std::size_t totNumberOfDetectedMutation = 0;

  /**
   * @brief total number of Build Failure
   */
  std::size_t totNumberOfBuildFailure = 0;

  /**
   * @brief total number of Runtime Error
   */
  std::size_t totNumberOfRuntimeError = 0;

  /**
   * @brief total number of Time Out
   */
  std::size_t totNumberOfTimeout = 0;

  /**
   * @brief path of source directory
   */
  std::experimental::filesystem::path mSourcePath;

  /**
   * @brief Report's logger
   */
  std::shared_ptr<Logger> mLogger;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_REPORT_HPP_
