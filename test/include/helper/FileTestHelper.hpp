/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef TEST_INCLUDE_HELPER_FILETESTHELPER_HPP_
#define TEST_INCLUDE_HELPER_FILETESTHELPER_HPP_

#include <filesystem>  // NOLINT
#include <fstream>
#include <string>

namespace sentinel {
namespace testutil {

/**
 * @brief Write content to a file, creating parent directories as needed.
 */
inline void writeFile(const std::filesystem::path& p, const std::string& content) {
  std::filesystem::create_directories(p.parent_path());
  std::ofstream f(p);
  f << content;
}

/**
 * @brief Read the entire contents of a file into a string.
 */
inline std::string readFile(const std::filesystem::path& p) {
  std::ifstream f(p);
  return {std::istreambuf_iterator<char>(f), {}};
}

}  // namespace testutil
}  // namespace sentinel

#endif  // TEST_INCLUDE_HELPER_FILETESTHELPER_HPP_
