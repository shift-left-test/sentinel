/*
 * Copyright (c) 2021 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <cerrno>
#include <cstring>
#include <filesystem>  // NOLINT
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "sentinel/CoverageInfo.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

namespace fs = std::filesystem;

namespace {

constexpr std::string_view kSfPrefix = "SF:";
constexpr std::string_view kDaPrefix = "DA:";

}  // namespace

CoverageInfo::CoverageInfo(const std::vector<fs::path>& filenames) {
  for (const auto& tracefile : filenames) {
    const std::string filename = tracefile.string();
    if (!fs::exists(tracefile)) {
      throw InvalidArgumentException(fmt::format(
          "--lcov-tracefile '{}' is still missing after build and test "
          "completed.\n"
          "       Ensure your build or test command generates this file "
          "(e.g., 'lcov --capture -d . -o {}').",
          filename, tracefile.filename().string()));
    }

    std::ifstream coverageFile(tracefile);
    if (!coverageFile) {
      throw InvalidArgumentException(fmt::format(
          "--lcov-tracefile '{}': failed to open ({}).",
          filename, std::strerror(errno)));
    }

    std::string line;
    std::string currentFile;
    std::size_t lineNo = 0;
    std::size_t skippedSfCount = 0;
    std::size_t skippedDaCount = 0;

    auto skipSf = [&](const std::string& reason) {
      Logger::verbose("--lcov-tracefile '{}' line {}: {} - skipping this block.",
                      filename, lineNo, reason);
      currentFile.clear();
      ++skippedSfCount;
    };
    auto skipDa = [&](const std::string& reason) {
      Logger::verbose("--lcov-tracefile '{}' line {}: {} - skipping.",
                      filename, lineNo, reason);
      ++skippedDaCount;
    };

    while (std::getline(coverageFile, line)) {
      ++lineNo;
      if (line.empty()) continue;

      if (string::startsWith(line, std::string(kSfPrefix))) {
        const std::string raw = line.substr(kSfPrefix.size());
        if (raw.empty()) {
          skipSf("SF record has empty path");
          continue;
        }
        std::error_code ec;
        const fs::path canon = fs::canonical(raw, ec);
        if (ec) {
          skipSf(fmt::format(
              "SF source '{}' cannot be resolved in the current source tree",
              raw));
          continue;
        }
        currentFile = canon.string();
        continue;
      }

      if (string::startsWith(line, std::string(kDaPrefix))) {
        if (currentFile.empty()) {
          skipDa("DA record without a preceding (resolved) SF record");
          continue;
        }

        const std::string rest = line.substr(kDaPrefix.size());
        const auto comma = rest.find(',');
        if (comma == std::string::npos) {
          skipDa(fmt::format("malformed DA record '{}'", line));
          continue;
        }
        const std::string lineNumStr = rest.substr(0, comma);
        std::string hitStr = rest.substr(comma + 1);
        // 'DA:line,hit,checksum' variant - drop checksum
        const auto comma2 = hitStr.find(',');
        if (comma2 != std::string::npos) {
          hitStr.resize(comma2);
        }

        if (hitStr == "0") continue;

        std::size_t lineNum = 0;
        try {
          lineNum = string::to<std::size_t>(lineNumStr);
        } catch (const std::exception&) {
          skipDa(fmt::format(
              "line number '{}' is not an integer in DA record", lineNumStr));
          continue;
        }
        mData[currentFile].insert(lineNum);
      }
    }

    if (skippedSfCount > 0 || skippedDaCount > 0) {
      Logger::verbose(
          "--lcov-tracefile '{}': skipped {} SF block(s), {} DA record(s).",
          filename, skippedSfCount, skippedDaCount);
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
