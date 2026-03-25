/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef TEST_INCLUDE_HELPER_TESTTEMPDIR_HPP_
#define TEST_INCLUDE_HELPER_TESTTEMPDIR_HPP_

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <string>

namespace sentinel {

/**
 * @brief Return a unique temp directory path for the running test.
 *
 * Combines @p prefix with the current test name so that tests from the
 * same fixture can run in parallel via `ctest -j` without colliding.
 *
 * @param prefix  Directory-name prefix (e.g. "SENTINEL_WORKSPACE_TEST").
 * @return  `temp_directory_path() / "<prefix>_<test_name>"`.
 */
inline std::filesystem::path testTempDir(const std::string& prefix) {
  const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
  return std::filesystem::temp_directory_path() /
         (prefix + "_" + info->test_suite_name() + "_" + info->name());
}

}  // namespace sentinel

#endif  // TEST_INCLUDE_HELPER_TESTTEMPDIR_HPP_
