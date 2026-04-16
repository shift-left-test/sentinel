/*
 * Copyright (c) 2021 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

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
        fs::path p = fs::canonical(fs::path(v[1]));
        currentFile = p.string();
      }

      if (string::startsWith(line, "DA")) {
        std::vector<std::string> v1 = string::split(line, ':');
        std::vector<std::string> v2 = string::split(v1[1], ',');
        if (v2[1] != "0") {
          mData[currentFile].insert(string::to<size_t>(v2[0]));
        }
      }
    }
  }
}

bool CoverageInfo::cover(const std::string& filename, size_t line) const {
  auto it = mData.find(filename);
  if (it == mData.end()) {
    return false;
  }
  return it->second.count(line) != 0;
}

}  // namespace sentinel
