/*
 * Copyright (c) 2021 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include <vector>
#include "sentinel/CoverageInfo.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

namespace fs = std::filesystem;

CoverageInfo::CoverageInfo(const std::vector<std::string>& filenames) {
  for (const auto& filename : filenames) {
    if (!fs::exists(filename)) {
      throw InvalidArgumentException(fmt::format("Input coverage file does not exist: {}", filename));
    }

    std::ifstream coverageFile(filename.c_str());
    std::string line;
    std::string currentFile;

    while (std::getline(coverageFile, line)) {
      if (string::startsWith(line, "SF")) {
        std::vector<std::string> v = string::split(line, ':');
        fs::path p = fs::absolute(fs::path(v[1]));
        mData[p.string()] = std::vector<size_t>();
        currentFile = p.string();
      }

      if (string::startsWith(line, "DA")) {
        std::vector<std::string> v1 = string::split(line, ':');
        std::vector<std::string> v2 = string::split(v1[1], ',');
        if (v2[1] != "0") {
          mData[currentFile].push_back(string::to<size_t>(v2[0]));
        }
      }
    }
  }
}

bool CoverageInfo::cover(const std::string& filename, size_t line) const {
  auto it = mData.find(fs::absolute(fs::path(filename)).string());
  if (it == mData.end()) {
    return false;
  }
  return std::find(it->second.begin(), it->second.end(), line) != it->second.end();
}

}  // namespace sentinel
