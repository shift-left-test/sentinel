/*
 * Copyright (c) 2021 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <experimental/filesystem>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "sentinel/CoverageInfo.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

CoverageInfo::CoverageInfo(const std::vector<std::string>& filenames) {
  namespace fs = std::experimental::filesystem;
  for (const auto& filename : filenames) {
    if (!fs::exists(filename)) {
      throw InvalidArgumentException(
        fmt::format("Input coverage file does not exist: {}",
                    filename));
    }

    std::ifstream coverageFile(filename.c_str());
    std::string line;
    std::string currentFile;

    while (std::getline(coverageFile, line)) {
      if (line.substr(0, 2) == "SF") {
        std::vector<std::string> v = string::split(line, ':');
        fs::path p = fs::absolute(fs::path(v[1]));
        mData[p.string()] = std::vector<size_t>();
        currentFile = p.string();
      }

      if (line.substr(0, 2) == "DA") {
        std::vector<std::string> v1 = string::split(line, ':');
        std::vector<std::string> v2 = string::split(v1[1], ',');
        if (v2[1] != "0") {
          mData[currentFile].push_back(std::stoi(v2[0]));
        }
      }
    }
  }
}

bool CoverageInfo::cover(const std::string& filename, size_t line) {
  namespace fs = std::experimental::filesystem;
  fs::path p = fs::absolute(fs::path(filename));

  if (mData.find(p.string()) == mData.end()) {
    return false;
  }

  return std::find(mData[p.string()].begin(),
                   mData[p.string()].end(), line) != mData[p.string()].end();
}

}  // namespace sentinel
