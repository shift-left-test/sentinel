/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef TEST_INCLUDE_HELPER_SENTINELREPORTTESTBASE_HPP_
#define TEST_INCLUDE_HELPER_SENTINELREPORTTESTBASE_HPP_

#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include "sentinel/Logger.hpp"

namespace fs = std::filesystem;

namespace sentinel {

/**
 * @brief Common base fixture for ReportTest and HTMLReportTest.
 *
 * Sets up a temporary source-directory tree shared by both test suites and
 * cleans up the Logger cache in TearDown so that logger state does not leak
 * between test cases.
 */
class SentinelReportTestBase : public ::testing::Test {
 protected:
  /**
   * @brief Create the standard directory tree under a per-suite temp dir.
   *
   * @param tmpDirName  Name appended to fs::temp_directory_path() as the root.
   */
  void setUpDirectories(const std::string& tmpDirName) {
    BASE = fs::temp_directory_path() / tmpDirName;
    fs::remove_all(BASE);

    SOURCE_DIR = BASE / "SOURCE_DIR";
    fs::create_directories(SOURCE_DIR);
    NESTED_SOURCE_DIR = SOURCE_DIR / "NESTED_DIR1/NESTED_DIR";
    fs::create_directories(NESTED_SOURCE_DIR);
    NESTED_SOURCE_DIR2 = SOURCE_DIR / "NESTED_DIR2";
    fs::create_directories(NESTED_SOURCE_DIR2);

    TARGET_FULL_PATH = NESTED_SOURCE_DIR / "target1_veryVeryVeryVeryVerylongFilePath.cpp";
    TARGET_FULL_PATH2 = NESTED_SOURCE_DIR2 / "target2.cpp";
    TARGET_FULL_PATH3 = NESTED_SOURCE_DIR2 / "target3.cpp";
    TARGET_FULL_PATH4 = SOURCE_DIR / "target4.cpp";
  }

  /**
   * @brief Remove the temp tree and reset the Logger cache.
   */
  void tearDownBase() {
    fs::remove_all(BASE);
    Logger::setDefaultLevel(Logger::Level::OFF);
    Logger::clearCache();
  }

  fs::path BASE;
  fs::path SOURCE_DIR;
  fs::path NESTED_SOURCE_DIR;
  fs::path NESTED_SOURCE_DIR2;
  fs::path TARGET_FULL_PATH;
  fs::path TARGET_FULL_PATH2;
  fs::path TARGET_FULL_PATH3;
  fs::path TARGET_FULL_PATH4;
};

}  // namespace sentinel

#endif  // TEST_INCLUDE_HELPER_SENTINELREPORTTESTBASE_HPP_
