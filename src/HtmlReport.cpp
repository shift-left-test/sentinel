/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>
#include <filesystem>  // NOLINT
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "sentinel/HtmlReport.hpp"
#include "sentinel/MutationState.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/operators/MutationOperatorExpansion.hpp"
#include "sentinel/util/io.hpp"
#include "sentinel/util/string.hpp"
#include "sentinel/version.hpp"

namespace sentinel {

namespace fs = std::filesystem;

namespace {
constexpr const char* kLcovModeRestrict = "restrict generation";
constexpr const char* kLcovModeSkipUncovered = "skip uncovered evaluation";
}  // namespace

HtmlReport::HtmlReport(const MutationSummary& summary, const Config& config)
    : Report(summary), mConfig(config) {
  initMetadata();
}

HtmlReport::~HtmlReport() = default;

void HtmlReport::initMetadata() {
  mVersion = PROGRAM_VERSION;
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  struct tm tmBuf{};
  localtime_r(&time, &tmBuf);
  std::ostringstream oss;
  oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S");
  mTimestamp = oss.str();
}

std::string HtmlReport::jsonEscape(const std::string& s) {
  std::string result;
  result.reserve(s.size());
  for (unsigned char c : s) {
    switch (c) {
      case '"':  result += "\\\""; break;
      case '\\': result += "\\\\"; break;
      case '\n': result += "\\n"; break;
      case '\r': result += "\\r"; break;
      case '\t': result += "\\t"; break;
      default:
        if (c < 0x20) {
          result += fmt::format("\\u{:04x}", static_cast<unsigned int>(c));
        } else {
          result += static_cast<char>(c);
        }
        break;
    }
  }
  return result;
}

std::string HtmlReport::buildConfigJson() const {
  std::ostringstream o;
  o << "{";

  auto addStr = [&o, this](const std::string& key, const std::string& val,
                            bool comma) {
    if (comma) {
      o << ",";
    }
    o << "\"" << key << "\":\"" << jsonEscape(val) << "\"";
  };

  bool comma = false;
  if (!mConfig.sourceDir.empty()) {
    addStr("sourceDir", mConfig.sourceDir.string(), comma);
    comma = true;
  }
  if (!mConfig.buildCmd.empty()) {
    addStr("buildCmd", mConfig.buildCmd, comma);
    comma = true;
  }
  if (!mConfig.testCmd.empty()) {
    addStr("testCmd", mConfig.testCmd, comma);
    comma = true;
  }

  std::string fromLabel;
  if (mConfig.from && mConfig.uncommitted) {
    fromLabel = fmt::format("{} + uncommitted", *mConfig.from);
  } else if (mConfig.from) {
    fromLabel = *mConfig.from;
  } else if (mConfig.uncommitted) {
    fromLabel = "uncommitted";
  } else {
    fromLabel = "all";
  }
  addStr("from", fromLabel, comma);
  comma = true;

  addStr("generator", generatorToString(mConfig.generator), comma);

  if (mConfig.mutantsPerLine != 1) {
    addStr("mutantsPerLine",
           mConfig.mutantsPerLine == 0 ? "unlimited"
                                       : std::to_string(mConfig.mutantsPerLine),
           comma);
  }
  if (!mConfig.operators.empty()) {
    addStr("operators", string::join(", ", mConfig.operators), comma);
  }
  if (mConfig.limit > 0) {
    addStr("limit", std::to_string(mConfig.limit), comma);
  }
  if (mConfig.seed.has_value()) {
    addStr("seed", std::to_string(mConfig.seed.value()), comma);
  }
  if (mConfig.threshold.has_value()) {
    addStr("threshold",
           fmt::format("{}%", static_cast<int>(mConfig.threshold.value())),
           comma);
  }
  if (mConfig.timeout.has_value()) {
    addStr("timeout", fmt::format("{}s", mConfig.timeout.value()), comma);
  }
  if (mConfig.partition.has_value()) {
    addStr("partition", mConfig.partition.value(), comma);
  }
  if (!mConfig.lcovTracefiles.empty()) {
    std::vector<std::string> paths;
    paths.reserve(mConfig.lcovTracefiles.size());
    std::transform(mConfig.lcovTracefiles.begin(), mConfig.lcovTracefiles.end(),
                   std::back_inserter(paths),
                   [](const std::filesystem::path& p) { return p.string(); });
    addStr("lcovTracefiles", string::join(", ", paths), comma);
    addStr("lcovMode",
           mConfig.restrictGeneration ? kLcovModeRestrict : kLcovModeSkipUncovered,
           comma);
  }

  o << "}";
  return o.str();
}

std::string HtmlReport::buildJsonData() const {
  std::ostringstream o;
  o << "{";
  o << "\"version\":\"" << jsonEscape(mVersion) << "\",";
  o << "\"timestamp\":\"" << jsonEscape(mTimestamp) << "\",";
  o << "\"config\":" << buildConfigJson() << ",";

  // --- summary ---
  o << "\"summary\":{";
  o << "\"totalMutations\":" << mSummary.totNumberOfMutation << ",";
  o << "\"detectedMutations\":" << mSummary.totNumberOfDetectedMutation << ",";
  o << "\"buildFailures\":" << mSummary.totNumberOfBuildFailure << ",";
  o << "\"runtimeErrors\":" << mSummary.totNumberOfRuntimeError << ",";
  o << "\"timeouts\":" << mSummary.totNumberOfTimeout << ",";
  o << "\"totalBuildSecs\":" << fmt::format("{:.2f}", mSummary.totalBuildSecs)
    << ",";
  o << "\"totalTestSecs\":" << fmt::format("{:.2f}", mSummary.totalTestSecs)
    << ",";

  // byOperator
  const std::vector<std::string> kOperatorOrder =
      {"AOR", "BOR", "LCR", "ROR", "SDL", "SOR", "UOI"};
  std::map<std::string, std::array<std::size_t, 3>> opMap;
  for (const auto& mr : mSummary.results) {
    const auto& op = mr.getMutant().getOperator();
    auto state = mr.getMutationState();
    if (state == MutationState::KILLED) {
      opMap[op][0]++;
    } else if (state == MutationState::SURVIVED) {
      opMap[op][1]++;
    } else {
      opMap[op][2]++;
    }
  }
  o << "\"byOperator\":{";
  bool firstOp = true;
  for (const auto& op : kOperatorOrder) {
    auto it = opMap.find(op);
    if (it == opMap.end()) {
      continue;
    }
    if (!firstOp) {
      o << ",";
    }
    firstOp = false;
    o << "\"" << op << "\":[" << it->second[0] << "," << it->second[1] << ","
      << it->second[2] << "]";
  }
  o << "},";

  // timeByState
  const std::vector<std::pair<std::string, MutationState>> kStateOrder = {
      {"KILLED", MutationState::KILLED},
      {"SURVIVED", MutationState::SURVIVED},
      {"TIMEOUT", MutationState::TIMEOUT},
      {"BUILD_FAILURE", MutationState::BUILD_FAILURE},
      {"RUNTIME_ERROR", MutationState::RUNTIME_ERROR},
  };
  o << "\"timeByState\":{";
  bool firstState = true;
  for (const auto& [label, state] : kStateOrder) {
    auto it = mSummary.timeByState.find(state);
    if (it == mSummary.timeByState.end() || it->second.count == 0) {
      continue;
    }
    if (!firstState) {
      o << ",";
    }
    firstState = false;
    o << "\"" << label << "\":{\"buildSecs\":"
      << fmt::format("{:.2f}", it->second.buildSecs)
      << ",\"testSecs\":" << fmt::format("{:.2f}", it->second.testSecs)
      << ",\"count\":" << it->second.count << "}";
  }
  o << "}";
  o << "},";  // end summary

  // --- dirs ---
  o << "\"dirs\":{";
  bool firstDir = true;
  for (const auto& [dirPath, dirStats] : mSummary.groupByDirPath) {
    if (!firstDir) {
      o << ",";
    }
    firstDir = false;

    std::size_t dirTimeout = 0;
    std::size_t dirBuildFailure = 0;
    std::size_t dirRuntimeError = 0;
    for (const auto* mr : dirStats.results) {
      auto state = mr->getMutationState();
      if (state == MutationState::TIMEOUT) {
        dirTimeout++;
      } else if (state == MutationState::BUILD_FAILURE) {
        dirBuildFailure++;
      } else if (state == MutationState::RUNTIME_ERROR) {
        dirRuntimeError++;
      }
    }

    o << "\"" << jsonEscape(dirPath.string()) << "\":{";
    o << "\"total\":" << dirStats.total << ",";
    o << "\"detected\":" << dirStats.detected << ",";
    o << "\"fileCount\":" << dirStats.fileCount << ",";
    o << "\"timeouts\":" << dirTimeout << ",";
    o << "\"buildFailures\":" << dirBuildFailure << ",";
    o << "\"runtimeErrors\":" << dirRuntimeError;
    o << "}";
  }
  o << "},";

  // --- files ---
  o << "\"files\":{";
  bool firstFile = true;
  for (const auto& [filePath, fileStats] : mSummary.groupByPath) {
    if (!firstFile) {
      o << ",";
    }
    firstFile = false;

    auto absSrcPath = mSummary.sourcePath / filePath;
    if (!fs::exists(absSrcPath)) {
      throw InvalidArgumentException(
          fmt::format("Source doesn't exist: {0}", absSrcPath));
    }

    std::ifstream tf(absSrcPath.string());
    std::stringstream buffer;
    buffer << tf.rdbuf();
    std::string srcContents = buffer.str();
    tf.close();

    auto srcLines = string::split(srcContents, '\n');

    o << "\"" << jsonEscape(filePath.string()) << "\":{";
    o << "\"total\":" << fileStats.total << ",";
    o << "\"detected\":" << fileStats.detected << ",";

    // source lines
    o << "\"source\":[";
    for (std::size_t i = 0; i < srcLines.size(); ++i) {
      if (i > 0) {
        o << ",";
      }
      o << "\"" << jsonEscape(srcLines[i]) << "\"";
    }
    o << "],";

    // mutations
    o << "\"mutations\":[";
    bool firstMut = true;
    for (const auto* mr : fileStats.results) {
      if (!firstMut) {
        o << ",";
      }
      firstMut = false;

      const auto& mutant = mr->getMutant();
      auto first = mutant.getFirst();
      auto last = mutant.getLast();

      if (first.line == 0) {
        throw InvalidArgumentException(
            fmt::format("Mutation at line number 0"));
      }
      if (first.line > srcLines.size()) {
        throw InvalidArgumentException(fmt::format(
            "Src file's line num({0}) is smaller than mutation' line num({1})",
            srcLines.size(), first.line));
      }

      // Build oriCode and mutCode
      std::string oriCode;
      std::string mutatedCodeHead;
      std::string mutatedCodeTail;
      for (std::size_t i = first.line; i <= last.line; i++) {
        std::string curLineContent = srcLines[i - 1];
        if (!oriCode.empty()) {
          oriCode += "\n";
        }
        if (i == first.line) {
          mutatedCodeHead = curLineContent.substr(0, first.column - 1);
        }
        if (i == last.line) {
          mutatedCodeTail = curLineContent.substr(
              last.column - 1, std::string::npos);
        }
        oriCode.append(curLineContent);
      }
      std::string mutCode;
      mutCode.append(mutatedCodeHead);
      mutCode.append(mutant.getToken());
      mutCode.append(mutatedCodeTail);

      const auto& opAbbrev = mutant.getOperator();
      std::string opFull = mutationOperatorToExpansion(opAbbrev);

      o << "{";
      o << "\"op\":\"" << jsonEscape(opAbbrev) << "\",";
      o << "\"opFull\":\"" << jsonEscape(opFull) << "\",";
      o << "\"firstLine\":" << first.line << ",";
      o << "\"firstCol\":" << first.column << ",";
      o << "\"lastLine\":" << last.line << ",";
      o << "\"lastCol\":" << last.column << ",";
      o << "\"token\":\"" << jsonEscape(mutant.getToken()) << "\",";
      o << "\"state\":\"" << mutationStateToStr(mr->getMutationState())
        << "\",";
      o << "\"killingTest\":\"" << jsonEscape(mr->getKillingTest()) << "\",";
      o << "\"oriCode\":\"" << jsonEscape(oriCode) << "\",";
      o << "\"mutCode\":\"" << jsonEscape(mutCode) << "\"";
      o << "}";
    }
    o << "]";

    o << "}";
  }
  o << "}";

  o << "}";
  return o.str();
}

void HtmlReport::save(const std::filesystem::path& dirPath) {
  io::ensureDirectoryExists(dirPath);

  std::string jsonData = buildJsonData();

  auto htmlPath = dirPath / "index.html";
  std::ofstream ofs(htmlPath, std::ofstream::out);
  if (!ofs) {
    throw IOException(errno, fmt::format("Failed to open '{}': {}",
                                         htmlPath.string(), std::strerror(errno)));
  }
  ofs << R"(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
  <title>Sentinel — Mutation Testing Report</title>
  <style>
)" << R"CSS(/* ============================================================
   Sentinel Mutation Testing Report
   Palette: Deep Teal + Vivid accents
   Base tones: #036098 (accent), #002030 (text)
   ============================================================ */

:root {
  --bg:          #f2f4f7;
  --bg-white:    #ffffff;
  --bg-muted:    #e8ecf1;
  --border:      #d0d5de;
  --border-light:#dfe3ea;

  --text:        #002030;
  --text-sec:    #2c4050;
  --text-muted:  #607080;

  --green:       #0f8a5f;
  --green-bg:    #d0eddb;
  --red:         #d44030;
  --red-bg:      #f8d8d2;
  --skip:        #94a0b0;
  --skip-bg:     #eef0f4;

  --accent:      #036098;
  --accent-light:#0880c8;
  --accent-hover:#024a76;
  --accent-bg:   #e8f2fa;

  --font:        -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
  --font-mono:   SFMono-Regular, Consolas, 'Liberation Mono', Menlo, monospace;
  --radius:      8px;
  --radius-sm:   5px;
}

*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
html { font-size: 15px; -webkit-font-smoothing: antialiased; }
body { font-family: var(--font); background: var(--bg); color: var(--text); line-height: 1.6; }
.wrap { max-width: 1140px; margin: 0 auto; padding: 28px 24px 48px; }

/* ================================================================
   HEADER
   ================================================================ */
.hdr {
  display: flex; align-items: center; justify-content: space-between;
  padding-bottom: 20px; border-bottom: 2px solid var(--border); margin-bottom: 24px;
}
.hdr__left { display: flex; align-items: center; gap: 10px; }
.hdr__icon { width: 30px; height: 30px; flex-shrink: 0; }
.hdr h1 { font-size: 1.12rem; font-weight: 700; color: var(--text); }
.hdr__right { display: flex; gap: 10px; }
.badge {
  font-size: .74rem; font-weight: 600; color: var(--text-sec);
  background: var(--bg-muted); border: 1px solid var(--border);
  border-radius: 16px; padding: 4px 12px;
}

/* ================================================================
   BREADCRUMB
   ================================================================ */
.crumb { font-size: .84rem; color: var(--text-muted); margin-bottom: 20px; }
.crumb a { color: var(--accent); text-decoration: none; font-weight: 600; cursor: pointer; }
.crumb a:hover { text-decoration: underline; color: var(--accent-hover); }
.crumb__sep { margin: 0 5px; opacity: .4; }

/* ================================================================
   CARDS
   ================================================================ */
.cards { display: grid; gap: 14px; margin-bottom: 24px; }
.cards--4 { grid-template-columns: repeat(4, 1fr); }
.cards--3 { grid-template-columns: repeat(3, 1fr); }

.card {
  background: var(--bg-white); border: 1px solid var(--border);
  border-radius: var(--radius); padding: 18px 20px;
  border-top: 3px solid transparent;
}
.card--score  { border-top-color: var(--accent); }
.card--killed { border-top-color: var(--green); }
.card--surv   { border-top-color: var(--red); }
.card--time   { border-top-color: var(--skip); }
.card--skip   { border-top-color: #6b7585; }
.card--skip   .card__val { color: #6b7585; }

.card__lbl {
  font-size: .72rem; font-weight: 700; text-transform: uppercase;
  letter-spacing: .06em; color: var(--text-sec); margin-bottom: 6px;
}
.card__val { font-size: 1.65rem; font-weight: 800; letter-spacing: -.02em; line-height: 1.15; }
.card--score  .card__val { color: var(--accent); }
.card--killed .card__val { color: var(--green); }
.card--surv   .card__val { color: var(--red); }
.card--time   .card__val { color: var(--text); }
.card__sub { font-size: .78rem; color: var(--text-sec); margin-top: 5px; }

.mini-bar { margin-top: 10px; height: 5px; background: var(--bg-muted); border-radius: 3px; overflow: hidden; }
.mini-bar__fill { height: 100%; border-radius: 3px; background: var(--accent); }

/* ================================================================
   PANELS
   ================================================================ */
.row2 { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; margin-bottom: 24px; }

.panel {
  background: var(--bg-white); border: 1px solid var(--border);
  border-radius: var(--radius); padding: 20px;
}
.panel__t {
  font-size: .74rem; font-weight: 700; text-transform: uppercase;
  letter-spacing: .06em; color: var(--text-sec); margin-bottom: 16px;
}

/* donut */
.donut-wrap { display: flex; align-items: center; gap: 28px; }
.donut-svg { width: 140px; height: 140px; flex-shrink: 0; }
.legend { display: flex; flex-direction: column; gap: 10px; }
.legend-i { display: flex; align-items: center; gap: 8px; font-size: .86rem; color: var(--text); }
.legend-dot { width: 10px; height: 10px; border-radius: 50%; flex-shrink: 0; }
.legend-i .n { font-weight: 700; margin-left: auto; min-width: 24px; text-align: right; }

/* bar chart */
.bars { display: flex; flex-direction: column; gap: 12px; }
.bar-hdr { display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 4px; }
.bar-name { font-size: .84rem; font-weight: 600; color: var(--text-muted); }
.bar-cnt { font-size: .74rem; color: var(--text-muted); }
.bar-track { height: 7px; background: var(--bg-muted); border-radius: 4px; overflow: hidden; display: flex; }
.bar-k { height: 100%; background: var(--green); }
.bar-s { height: 100%; background: var(--red); }
.bar-x { height: 100%; background: var(--skip); }
.bar-build { height: 100%; background: #036098; }
.bar-test  { height: 100%; background: #48b0e0; }

/* ================================================================
   CONFIG
   ================================================================ */
.cfg-btn {
  width: 100%; background: var(--bg-white); border: 1px solid var(--border);
  border-radius: var(--radius); padding: 13px 18px; color: var(--text-sec);
  font-family: var(--font); font-size: .78rem; font-weight: 700;
  text-transform: uppercase; letter-spacing: .06em;
  cursor: pointer; display: flex; align-items: center; justify-content: space-between;
}
.cfg-btn:hover { background: var(--bg-muted); }
.cfg-btn__arr { transition: transform .2s; font-size: .65rem; }
.cfg-btn[aria-expanded="true"] .cfg-btn__arr { transform: rotate(180deg); }
.cfg-btn[aria-expanded="true"] { border-radius: var(--radius) var(--radius) 0 0; }

.cfg-body {
  background: var(--bg-white); border: 1px solid var(--border); border-top: none;
  border-radius: 0 0 var(--radius) var(--radius);
  max-height: 0; overflow: hidden; transition: max-height .3s ease, padding .3s ease;
  padding: 0 18px;
}
.cfg-body.open { max-height: 500px; padding: 14px 18px 18px; }

.cfg-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 6px 28px; }
.cfg-item { display: flex; gap: 8px; font-size: .84rem; padding: 5px 0; border-bottom: 1px solid var(--border-light); }
.cfg-item:last-child { border-bottom: none; }
.cfg-k { color: var(--text-muted); font-weight: 600; min-width: 100px; font-family: var(--font-mono); font-size: .76rem; }
.cfg-v { color: var(--text); font-family: var(--font-mono); font-size: .8rem; word-break: break-all; }

/* ================================================================
   TABLE
   ================================================================ */
.tbl-sec { margin-top: 24px; }
.sec-t {
  font-size: .76rem; font-weight: 700; text-transform: uppercase;
  letter-spacing: .06em; color: var(--text-sec); margin-bottom: 12px;
}

.dtbl {
  width: 100%; border-collapse: separate; border-spacing: 0;
  background: var(--bg-white); border: 1px solid var(--border);
  border-radius: var(--radius); overflow: hidden;
}
.dtbl th {
  font-size: .72rem; font-weight: 700; text-transform: uppercase;
  letter-spacing: .06em; color: var(--text-muted);
  padding: 10px 14px; text-align: left;
  background: var(--bg-muted); border-bottom: 1px solid var(--border);
}
.dtbl td {
  padding: 10px 14px; font-size: .86rem; color: var(--text);
  border-bottom: 1px solid var(--border-light); vertical-align: middle;
}
.dtbl tr:last-child td { border-bottom: none; }
.dtbl tr:hover td { background: var(--bg-muted); }
.dtbl a { color: var(--accent); text-decoration: none; font-weight: 600; cursor: pointer; }
.dtbl a:hover { text-decoration: underline; color: var(--accent-hover); }

.cov-cell { display: flex; align-items: center; gap: 8px; }
.cov-pct { font-family: var(--font-mono); font-weight: 600; font-size: .84rem; min-width: 38px; text-align: right; }
.cov-bar { flex: 1; height: 7px; background: var(--bg-muted); border-radius: 4px; overflow: hidden; max-width: 160px; }
.cov-bar__fill { height: 100%; border-radius: 4px; }
.cov-ratio { font-size: .76rem; color: var(--text-muted); font-family: var(--font-mono); min-width: 32px; }

.c-hi  { color: var(--green); }
.c-mid { color: #c07b10; }
.c-lo  { color: var(--red); }
.f-hi  { background: var(--green); }
.f-mid { background: #d99020; }
.f-lo  { background: var(--red); }

/* ================================================================
   SOURCE CODE
   ================================================================ */
.src-tbl {
  width: 100%; border-collapse: collapse;
  font-family: var(--font-mono); font-size: .84rem;
  background: var(--bg-white); border: 1px solid var(--border);
  border-radius: var(--radius); overflow: hidden;
}
.src-tbl tr:hover { background: var(--bg-muted); }

.src-tbl .ln {
  width: 44px; text-align: right; padding: 2px 8px 2px 6px;
  color: var(--text-muted); background: var(--bg-muted);
  border-right: 1px solid var(--border); user-select: none; font-size: .8rem;
}
.src-tbl .mi {
  width: 32px; text-align: center; padding: 2px 4px;
  font-size: .74rem; font-weight: 700; cursor: pointer;
}
.src-tbl .cd { padding: 2px 10px; white-space: pre; color: var(--text); }
.src-tbl .cd pre { margin: 0; font-family: inherit; font-size: inherit; }

.lk .mi { background: var(--green-bg); color: var(--green); }
.lk .cd { background: var(--green-bg); }
.ls .mi { background: var(--red-bg); color: var(--red); }
.ls .cd { background: var(--red-bg); }

/* popup */
.mpop {
  display: none; position: fixed; z-index: 1000;
  background: var(--bg-white); border: 1px solid var(--border);
  border-radius: var(--radius); padding: 14px 18px;
  width: 400px; max-height: 70vh; overflow-y: auto;
  box-shadow: 0 8px 30px rgba(0,32,48,.14);
  font-family: var(--font); font-weight: 400; text-align: left; line-height: 1.6;
}
.mpop.visible { display: block; }
.mpop__t { font-weight: 700; font-size: .84rem; margin-bottom: 8px; display: flex; align-items: center; gap: 6px; }
.mpop__badge { font-size: .66rem; padding: 2px 8px; border-radius: 8px; font-weight: 700; text-transform: uppercase; }
.b-k { background: var(--green-bg); color: var(--green); }
.b-s { background: var(--red-bg); color: var(--red); }
.mpop__lbl { font-size: .72rem; color: var(--text-muted); text-transform: uppercase; letter-spacing: .04em; margin-top: 6px; }
.mpop__code { background: var(--bg-muted); border-radius: var(--radius-sm); padding: 6px 10px; margin: 4px 0; font-family: var(--font-mono); font-size: .8rem; color: var(--text); }
.mpop__sep { border: none; border-top: 1px solid var(--border-light); margin: 10px 0; }

/* ================================================================
   MUTATIONS LIST
   ================================================================ */
.mlist { display: flex; flex-direction: column; gap: 6px; }
.ment {
  display: grid; grid-template-columns: minmax(28px, auto) minmax(220px, auto) minmax(0, 1fr) auto;
  align-items: center; column-gap: 10px;
  padding: 8px 12px; background: var(--bg-white); border: 1px solid var(--border);
  border-radius: var(--radius-sm); font-size: .84rem; color: var(--text);
}
.ment:hover { background: var(--bg-muted); }
.ment__ln { font-family: var(--font-mono); font-weight: 600; color: var(--accent); min-width: 28px; }
.ment__op { font-family: var(--font-mono); font-weight: 500; }
.ment__kl {
  min-width: 0; font-size: .78rem; color: var(--text-muted);
  text-align: right; justify-self: end; overflow-wrap: anywhere;
}
.ment__st {
  font-size: .72rem; font-weight: 700; text-transform: uppercase;
  padding: 2px 8px; border-radius: 8px; justify-self: end;
}

.tags { display: flex; flex-wrap: wrap; gap: 5px; margin-top: 6px; }
.tag {
  font-size: .76rem; font-family: var(--font-mono); padding: 3px 10px;
  border-radius: 12px; background: var(--bg-muted); border: 1px solid var(--border); color: var(--text-sec);
}

/* ================================================================
   FOOTER
   ================================================================ */
.ftr {
  margin-top: 36px; padding-top: 16px; border-top: 2px solid var(--border);
  font-size: .76rem; color: var(--text-muted); text-align: center;
}
.ftr a { color: var(--accent); text-decoration: none; font-weight: 600; }
.ftr a:hover { text-decoration: underline; }

@media (max-width: 900px) {
  .cards--4 { grid-template-columns: repeat(2,1fr); }
  .cards--3 { grid-template-columns: repeat(2,1fr); }
  .row2 { grid-template-columns: 1fr; }
  .cfg-grid { grid-template-columns: 1fr; }
}
@media (max-width: 600px) {
  .cards--4, .cards--3 { grid-template-columns: 1fr; }
  .hdr { flex-direction: column; gap: 10px; align-items: flex-start; }
}
)CSS" << R"(</style>
</head>
<body>
<div id="app"></div>
<script>var DATA = )" << jsonData << R"JS(;</script>
<script>
(function(){
var D = DATA;
var app = document.getElementById('app');

function h(s) {
  return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;')
    .replace(/"/g,'&quot;').replace(/'/g,'&#39;');
}

function formatDuration(secs) {
  if (secs < 60) return Math.round(secs) + 's';
  var m = Math.floor(secs / 60);
  var s = Math.floor(secs) % 60;
  if (s === 0) return m + 'm';
  return m + 'm ' + s + 's';
}

function covClass(cov) { return cov >= 70 ? 'c-hi' : cov >= 40 ? 'c-mid' : 'c-lo'; }
function fillClass(cov) { return cov >= 70 ? 'f-hi' : cov >= 40 ? 'f-mid' : 'f-lo'; }

function nav(hash) { location.hash = hash; }

function formatSkippedDetail(to, bf, re) {
  var parts = [];
  if (to > 0) parts.push(to + ' timeout');
  if (bf > 0) parts.push(bf + ' build failure');
  if (re > 0) parts.push(re + ' runtime error');
  return parts.join(' \u00b7 ');
}

function buildCards(score, killed, survived, skipped, valid, skippedDetail) {
  return '<section class="cards cards--4">' +
    '<div class="card card--score"><div class="card__lbl">Mutation Score</div>' +
    '<div class="card__val">' + score + '%</div>' +
    '<div class="card__sub">' + killed + ' / ' + valid + ' valid mutants</div>' +
    '<div class="mini-bar"><div class="mini-bar__fill" style="width:' + score + '%"></div></div></div>' +
    '<div class="card card--killed"><div class="card__lbl">Killed</div>' +
    '<div class="card__val">' + killed + '</div><div class="card__sub">Detected by tests</div></div>' +
    '<div class="card card--surv"><div class="card__lbl">Survived</div>' +
    '<div class="card__val">' + survived + '</div><div class="card__sub">Not detected</div></div>' +
    '<div class="card card--skip"><div class="card__lbl">Skipped</div>' +
    '<div class="card__val">' + skipped + '</div>' +
    '<div class="card__sub">' + (skippedDetail || '') + '</div></div></section>';
}

function donutSvg(segments, centerText, centerLabel) {
  var R = 54, C = 2 * Math.PI * R;
  var s = '<svg class="donut-svg" viewBox="0 0 140 140">' +
    '<circle cx="70" cy="70" r="54" fill="none" stroke="#e8ecf1" stroke-width="15"/>';
  var offset = C / 4;
  for (var i = 0; i < segments.length; i++) {
    var arc = segments[i].frac * C;
    s += '<circle cx="70" cy="70" r="54" fill="none" stroke="' + segments[i].color +
      '" stroke-width="15" stroke-dasharray="' + arc.toFixed(1) + ' ' + (C - arc).toFixed(1) +
      '" stroke-dashoffset="' + offset.toFixed(1) +
      '" style="transform:rotate(-90deg);transform-origin:center;"/>';
    offset -= arc;
  }
  var fs = String(centerText).length > 5 ? '16' : String(centerText).length > 3 ? '18' : '22';
  s += '<text x="70" y="66" text-anchor="middle" fill="#002030" font-family="system-ui,sans-serif"' +
    ' font-size="' + fs + '" font-weight="800">' + h(centerText) + '</text>' +
    '<text x="70" y="80" text-anchor="middle" fill="#607080" font-family="system-ui,sans-serif"' +
    ' font-size="10" font-weight="600">' + h(centerLabel) + '</text></svg>';
  return s;
}

function legendItem(color, label, value) {
  return '<div class="legend-i"><span class="legend-dot" style="background:' + color +
    '"></span>' + h(label) + '<span class="n">' + h(value) + '</span></div>';
}

function barRow(name, cntHtml, barSegments) {
  var bars = '';
  for (var i = 0; i < barSegments.length; i++) {
    bars += '<div class="' + barSegments[i].cls + '" style="width:' +
      barSegments[i].pct.toFixed(1) + '%"></div>';
  }
  return '<div><div class="bar-hdr"><span class="bar-name">' + h(name) +
    '</span><span class="bar-cnt">' + cntHtml + '</span></div>' +
    '<div class="bar-track">' + bars + '</div></div>';
}

var opOrder = ['AOR','BOR','LCR','ROR','SDL','SOR','UOI'];
var opNames = {
  'AOR':'AOR (Arithmetic Operator Replacement)',
  'BOR':'BOR (Bitwise Operator Replacement)',
  'LCR':'LCR (Logical Connector Replacement)',
  'ROR':'ROR (Relational Operator Replacement)',
  'SDL':'SDL (Statement Deletion)',
  'SOR':'SOR (Shift Operator Replacement)',
  'UOI':'UOI (Unary Operator Insertion)'
};
var stateOrder = [
  ['Killed','KILLED'], ['Survived','SURVIVED'], ['Timeout','TIMEOUT'],
  ['Build Failure','BUILD_FAILURE'], ['Runtime Error','RUNTIME_ERROR']
];

function renderRoot() {
  var sm = D.summary;
  if (sm.totalMutations === 0) {
    return '<div class="wrap"><header class="hdr"><div class="hdr__left">' +
      '<h1>Mutation Testing Report</h1></div></header>' +
      '<div class="panel" style="text-align:center;padding:40px">' +
      '<p style="color:var(--text-muted)">There is no mutated file in this project.</p></div>' +
      '<footer class="ftr">Report generated by <a href="https://github.com/shift-left-test/sentinel">' +
      'Sentinel</a></footer></div>';
  }
  var killed = sm.detectedMutations;
  var survived = sm.totalMutations - killed;
  var skipped = sm.buildFailures + sm.runtimeErrors + sm.timeouts;
  var total = killed + survived + skipped;
  var score = sm.totalMutations > 0 ? Math.floor(100 * killed / sm.totalMutations) : 0;
  var skippedDetail = formatSkippedDetail(sm.timeouts, sm.buildFailures, sm.runtimeErrors);

  var out = '<div class="wrap"><header class="hdr"><div class="hdr__left">' +
    '<h1>Mutation Testing Report</h1></div><div class="hdr__right">' +
    '<span class="badge">Generated: ' + h(D.timestamp) + '</span></div></header>';

  out += buildCards(score, killed, survived, skipped, sm.totalMutations, skippedDetail);

  // Panels row
  // Left: mutants donut + operator bars
  var mutSegs = [];
  if (total > 0) {
    mutSegs.push({frac: killed / total, color: '#0f8a5f'});
    mutSegs.push({frac: survived / total, color: '#d44030'});
    mutSegs.push({frac: skipped / total, color: '#94a0b0'});
  }
  var mutLegend = legendItem('#0f8a5f','Killed',killed) + legendItem('#d44030','Survived',survived);
  if (sm.timeouts > 0) mutLegend += legendItem('#94a0b0','Timeout',sm.timeouts);
  if (sm.buildFailures > 0) mutLegend += legendItem('#94a0b0','Build Failure',sm.buildFailures);
  if (sm.runtimeErrors > 0) mutLegend += legendItem('#94a0b0','Runtime Error',sm.runtimeErrors);

  var opBars = '';
  for (var oi = 0; oi < opOrder.length; oi++) {
    var op = opOrder[oi];
    var d = sm.byOperator[op];
    if (!d) continue;
    var ok = d[0], os = d[1], ox = d[2], ot = ok + os + ox;
    if (ot === 0) continue;
    var cnt = ok + ' killed';
    if (os > 0) cnt += ' \u00b7 ' + os + ' survived';
    if (ox > 0) cnt += ' \u00b7 ' + ox + ' skipped';
    var segs = [];
    if (ok > 0) segs.push({cls:'bar-k', pct: 100*ok/ot});
    if (os > 0) segs.push({cls:'bar-s', pct: 100*os/ot});
    if (ox > 0) segs.push({cls:'bar-x', pct: 100*ox/ot});
    opBars += barRow(opNames[op] || op, cnt, segs);
  }

  // Right: duration donut + state bars
  var totalSecs = sm.totalBuildSecs + sm.totalTestSecs;
  var durSegs = [];
  if (totalSecs > 0) {
    durSegs.push({frac: sm.totalBuildSecs / totalSecs, color: '#036098'});
    durSegs.push({frac: sm.totalTestSecs / totalSecs, color: '#48b0e0'});
  }
  var durLegend = legendItem('#036098','Build',formatDuration(sm.totalBuildSecs)) +
    legendItem('#48b0e0','Test',formatDuration(sm.totalTestSecs));

  var stBars = '';
  for (var si = 0; si < stateOrder.length; si++) {
    var sl = stateOrder[si][0], sk = stateOrder[si][1];
    var st = sm.timeByState[sk];
    if (!st || st.count === 0) continue;
    var stTotal = st.buildSecs + st.testSecs;
    var pct = totalSecs > 0 ? Math.round(stTotal / totalSecs * 100) : 0;
    var mw = st.count === 1 ? 'mutant' : 'mutants';
    var stCnt = pct + '% \u00b7 ' + formatDuration(stTotal) + ' (' + formatDuration(st.buildSecs) +
      ' / ' + formatDuration(st.testSecs) + ') \u00b7 ' + st.count + ' ' + mw;
    var stSegs = [];
    if (st.buildSecs > 0) stSegs.push({cls:'bar-build', pct: totalSecs > 0 ? st.buildSecs/totalSecs*100 : 0});
    if (st.testSecs > 0) stSegs.push({cls:'bar-test', pct: totalSecs > 0 ? st.testSecs/totalSecs*100 : 0});
    stBars += barRow(sl, stCnt, stSegs);
  }

  out += '<section class="row2"><div class="panel"><div class="panel__t">Mutants</div>' +
    '<div class="donut-wrap">' + donutSvg(mutSegs, total, 'TOTAL') +
    '<div class="legend">' + mutLegend + '</div></div>' +
    '<div class="panel__t" style="margin-top:24px">By Operator</div>' +
    '<div class="bars">' + opBars + '</div></div>' +
    '<div class="panel"><div class="panel__t">Duration</div>' +
    '<div class="donut-wrap">' + donutSvg(durSegs, formatDuration(totalSecs), 'TOTAL') +
    '<div class="legend">' + durLegend + '</div></div>' +
    '<div class="panel__t" style="margin-top:24px">By State</div>' +
    '<div class="bars">' + stBars + '</div></div></section>';

  // Config
  var cfg = D.config;
  var cfgKeys = Object.keys(cfg);
  if (cfgKeys.length > 0) {
    var items = '';
    for (var ci = 0; ci < cfgKeys.length; ci++) {
      items += '<div class="cfg-item"><span class="cfg-k">' + h(cfgKeys[ci]) +
        '</span><span class="cfg-v">' + h(cfg[cfgKeys[ci]]) + '</span></div>';
    }
    out += '<button class="cfg-btn" aria-expanded="false" onclick="this.setAttribute(\'aria-expanded\',' +
      'this.getAttribute(\'aria-expanded\')===\'true\'?\'false\':\'true\');' +
      'this.nextElementSibling.classList.toggle(\'open\')">' +
      '<span>Run Configuration</span><span class="cfg-btn__arr">&#9660;</span></button>' +
      '<div class="cfg-body"><div class="cfg-grid">' + items + '</div></div>';
  }

  // Directory table
  var dirs = D.dirs;
  var dirKeys = Object.keys(dirs);
  out += '<section class="tbl-sec"><div class="sec-t">Breakdown by File</div>' +
    '<table class="dtbl"><thead><tr><th>Name</th><th style="width:70px;text-align:center">Files</th>' +
    '<th style="width:300px">Mutation Score</th></tr></thead><tbody>';
  for (var di = 0; di < dirKeys.length; di++) {
    var dk = dirKeys[di];
    var dd = dirs[dk];
    var ds = dd.total > 0 ? Math.floor(100 * dd.detected / dd.total) : 0;
    var dn = dk === '' ? '.' : dk;
    out += '<tr><td><a onclick="nav(\'#/dir/' + encodeURIComponent(dk) + '\')">' + h(dn) + '</a></td>' +
      '<td style="text-align:center">' + dd.fileCount + '</td>' +
      '<td><div class="cov-cell"><span class="cov-pct ' + covClass(ds) + '">' + ds + '%</span>' +
      '<div class="cov-bar"><div class="cov-bar__fill ' + fillClass(ds) + '" style="width:' + ds + '%"></div></div>' +
      '<span class="cov-ratio">' + dd.detected + '/' + dd.total + '</span></div></td></tr>';
  }
  out += '</tbody></table></section>';

  out += '<footer class="ftr">Report generated by <a href="https://github.com/shift-left-test/sentinel">' +
    'Sentinel</a> v' + h(D.version) + '</footer></div>';
  return out;
}

function parentDir(filePath) {
  var idx = filePath.lastIndexOf('/');
  return idx >= 0 ? filePath.substring(0, idx) : '';
}

function fileName(filePath) {
  var idx = filePath.lastIndexOf('/');
  return idx >= 0 ? filePath.substring(idx + 1) : filePath;
}

function renderDir(dirPath) {
  dirPath = decodeURIComponent(dirPath);
  var dd = D.dirs[dirPath];
  if (!dd) return '<div class="wrap"><p>Directory not found.</p></div>';

  var killed = dd.detected;
  var total = dd.total;
  var survived = total - killed;
  var skipped = dd.timeouts + dd.buildFailures + dd.runtimeErrors;
  var score = total > 0 ? Math.floor(100 * killed / total) : 0;
  var skippedDetail = formatSkippedDetail(dd.timeouts, dd.buildFailures, dd.runtimeErrors);
  var displayName = dirPath === '' ? '.' : dirPath;

  var out = '<div class="wrap"><header class="hdr"><div class="hdr__left">' +
    '<h1>Mutation Testing Report</h1></div></header>';

  out += '<nav class="crumb"><a onclick="nav(\'#/\')">Project</a>' +
    '<span class="crumb__sep">/</span>' +
    '<span style="color:var(--text)">' + h(displayName) + '</span></nav>';

  out += buildCards(score, killed, survived, skipped, total, skippedDetail);

  // File table
  out += '<section class="tbl-sec"><div class="sec-t">Breakdown by File</div>' +
    '<table class="dtbl"><thead><tr><th>Name</th><th style="width:300px">Mutation Score</th></tr></thead><tbody>';

  var files = D.files;
  var fKeys = Object.keys(files);
  for (var fi = 0; fi < fKeys.length; fi++) {
    var fp = fKeys[fi];
    if (parentDir(fp) !== dirPath) continue;
    var fd = files[fp];
    var fs = fd.total > 0 ? Math.floor(100 * fd.detected / fd.total) : 0;
    out += '<tr><td><a onclick="nav(\'#/file/' + encodeURIComponent(fp) + '\')">' + h(fileName(fp)) + '</a></td>' +
      '<td><div class="cov-cell"><span class="cov-pct ' + covClass(fs) + '">' + fs + '%</span>' +
      '<div class="cov-bar"><div class="cov-bar__fill ' + fillClass(fs) + '" style="width:' + fs + '%"></div></div>' +
      '<span class="cov-ratio">' + fd.detected + '/' + fd.total + '</span></div></td></tr>';
  }
  out += '</tbody></table></section>';

  out += '<footer class="ftr">Report generated by <a href="https://github.com/shift-left-test/sentinel">' +
    'Sentinel</a></footer></div>';
  return out;
}

function renderFile(filePath) {
  filePath = decodeURIComponent(filePath);
  var fd = D.files[filePath];
  if (!fd) return '<div class="wrap"><p>File not found.</p></div>';

  var muts = fd.mutations;
  var fileKilled = 0, fileSurvived = 0, fileTimeout = 0, fileBF = 0, fileRE = 0;
  var uniqueTests = {}, uniqueOps = {};
  var groupByLine = {};

  for (var mi = 0; mi < muts.length; mi++) {
    var m = muts[mi];
    switch (m.state) {
      case 'KILLED': fileKilled++; break;
      case 'SURVIVED': fileSurvived++; break;
      case 'TIMEOUT': fileTimeout++; break;
      case 'BUILD_FAILURE': fileBF++; break;
      case 'RUNTIME_ERROR': fileRE++; break;
    }
    if (m.killingTest) {
      var tests = m.killingTest.split(', ');
      for (var ti = 0; ti < tests.length; ti++) {
        if (tests[ti]) uniqueTests[tests[ti]] = 1;
      }
    }
    uniqueOps[m.op] = 1;
    var ln = m.firstLine;
    if (!groupByLine[ln]) groupByLine[ln] = [];
    groupByLine[ln].push(m);
  }

  var fileSkipped = fileTimeout + fileBF + fileRE;
  var valid = fileKilled + fileSurvived;
  var score = valid > 0 ? Math.floor(100 * fileKilled / valid) : 0;
  var skippedDetail = formatSkippedDetail(fileTimeout, fileBF, fileRE);
  var srcName = fileName(filePath);
  var dirPath2 = parentDir(filePath);

  var out = '<div class="wrap"><header class="hdr"><div class="hdr__left">' +
    '<h1>Mutation Testing Report</h1></div></header>';

  // Breadcrumb
  var dirLink = '';
  if (dirPath2 && dirPath2 !== '.') {
    dirLink = '<span class="crumb__sep">/</span>' +
      '<a onclick="nav(\'#/dir/' + encodeURIComponent(dirPath2) + '\')">' + h(dirPath2) + '</a>';
  }
  out += '<nav class="crumb"><a onclick="nav(\'#/\')">Project</a>' + dirLink +
    '<span class="crumb__sep">/</span><span style="color:var(--text)">' + h(srcName) + '</span></nav>';

  // Cards
  var skSub = fileSkipped > 0 && skippedDetail ? skippedDetail : fileSkipped > 0 ? fileSkipped + ' skipped' : 'None';
  out += '<section class="cards cards--4">' +
    '<div class="card card--score"><div class="card__lbl">Mutation Score</div>' +
    '<div class="card__val">' + score + '%</div>' +
    '<div class="card__sub">' + fileKilled + ' / ' + valid + ' valid mutants</div>' +
    '<div class="mini-bar"><div class="mini-bar__fill" style="width:' + score + '%"></div></div></div>' +
    '<div class="card card--killed"><div class="card__lbl">Killed</div>' +
    '<div class="card__val">' + fileKilled + '</div><div class="card__sub">Detected by tests</div></div>' +
    '<div class="card card--surv"><div class="card__lbl">Survived</div>' +
    '<div class="card__val">' + fileSurvived + '</div><div class="card__sub">Not detected</div></div>' +
    '<div class="card card--skip"><div class="card__lbl">Skipped</div>' +
    '<div class="card__val">' + fileSkipped + '</div><div class="card__sub">' + skSub + '</div></div></section>';

  // Source code
  out += '<section><div class="sec-t">Source Code</div><table class="src-tbl">';
  var popups = '';
  var lines = fd.source;
  for (var li = 0; li < lines.length; li++) {
    var lineNum = li + 1;
    var lineMuts = groupByLine[lineNum];
    var rowClass = '';
    if (lineMuts) {
      var hasK = false, hasS = false;
      for (var lmi = 0; lmi < lineMuts.length; lmi++) {
        if (lineMuts[lmi].state === 'KILLED') hasK = true;
        else hasS = true;
      }
      if (hasK && hasS) rowClass = ' class="ls"';
      else if (hasK) rowClass = ' class="lk"';
      else if (hasS) rowClass = ' class="ls"';
    }

    var miCell;
    if (lineMuts && lineMuts.length > 0) {
      miCell = '<td class="mi" data-pop="pop-' + lineNum + '">' + lineMuts.length + '</td>';
    } else {
      miCell = '<td class="mi"></td>';
    }

    out += '<tr id="line-' + lineNum + '"' + rowClass + '>' +
      '<td class="ln">' + lineNum + '</td>' + miCell +
      '<td class="cd"><pre>' + h(lines[li]) + '</pre></td></tr>';

    // Popup
    if (lineMuts && lineMuts.length > 0) {
      var popContent = '';
      for (var pi = 0; pi < lineMuts.length; pi++) {
        if (pi > 0) popContent += '<hr class="mpop__sep"/>';
        var pm = lineMuts[pi];
        var isK = pm.state === 'KILLED';
        var bCls = isK ? 'b-k' : 'b-s';
        var bTxt = isK ? 'Killed' : 'Survived';
        var tDisp = pm.killingTest || 'none';
        var tColor = isK ? 'var(--green)' : 'var(--text-muted)';
        popContent += '<div class="mpop__t">' + h(pm.opFull) +
          ' <span class="mpop__badge ' + bCls + '">' + bTxt + '</span></div>' +
          '<div class="mpop__lbl">Original</div><div class="mpop__code">' + h(pm.oriCode) + '</div>' +
          '<div class="mpop__lbl">Mutated</div><div class="mpop__code">' + h(pm.mutCode) + '</div>' +
          '<div class="mpop__lbl" style="margin-top:8px">Killed by</div>' +
          '<div style="font-size:.82rem;color:' + tColor + ';margin-top:3px">' + h(tDisp) + '</div>';
      }
      popups += '<div id="pop-' + lineNum + '" class="mpop">' + popContent + '</div>';
    }
  }
  out += '</table></section>';
  out += popups;

  // Mutations list
  out += '<section style="margin-top:24px"><div class="sec-t">Mutants</div><div class="mlist">';
  var sortedLines = Object.keys(groupByLine).map(Number).sort(function(a,b){return a-b;});
  for (var sli = 0; sli < sortedLines.length; sli++) {
    var sln = sortedLines[sli];
    var slMuts = groupByLine[sln];
    for (var slmi = 0; slmi < slMuts.length; slmi++) {
      var sm2 = slMuts[slmi];
      var isK2 = sm2.state === 'KILLED';
      var bCls2 = isK2 ? 'b-k' : 'b-s';
      var bTxt2 = isK2 ? 'Killed' : 'Survived';
      var tDisp2 = sm2.killingTest || 'none';
      out += '<div class="ment"><a class="ment__ln" href="#line-' + sln + '">:' + sln + '</a>' +
        '<span class="ment__op">' + h(sm2.opFull) + '</span>' +
        '<span class="ment__kl">' + h(tDisp2) + '</span>' +
        '<span class="ment__st ' + bCls2 + '">' + bTxt2 + '</span></div>';
    }
  }
  out += '</div></section>';

  // Mutators and Tests panels
  var mutPanel = '', testPanel = '';
  var opKeys = [];
  for (var oi2 = 0; oi2 < opOrder.length; oi2++) {
    if (uniqueOps[opOrder[oi2]]) opKeys.push(opOrder[oi2]);
  }
  if (opKeys.length > 0) {
    mutPanel = '<div class="panel"><div class="panel__t">Active Mutators</div><div class="tags">';
    for (var oki = 0; oki < opKeys.length; oki++) {
      mutPanel += '<span class="tag">' + h(opNames[opKeys[oki]] || opKeys[oki]) + '</span>';
    }
    mutPanel += '</div></div>';
  }
  var testKeys = Object.keys(uniqueTests).sort();
  if (testKeys.length > 0) {
    testPanel = '<div class="panel"><div class="panel__t">Tests Examined</div><div class="tags">';
    for (var tki = 0; tki < testKeys.length; tki++) {
      testPanel += '<span class="tag">' + h(testKeys[tki]) + '</span>';
    }
    testPanel += '</div></div>';
  }
  if (mutPanel || testPanel) {
    out += '<section class="row2" style="margin-top:24px">' + mutPanel + testPanel + '</section>';
  }

  out += '<footer class="ftr">Report generated by <a href="https://github.com/shift-left-test/sentinel">' +
    'Sentinel</a> v' + h(D.version) + '</footer></div>';
  return out;
}

function initPopups() {
  var active = null;
  var miEls = document.querySelectorAll('.mi[data-pop]');
  miEls.forEach(function(mi) {
    mi.addEventListener('mouseenter', function() {
      var pop = document.getElementById(mi.getAttribute('data-pop'));
      if (!pop) return;
      if (active && active !== pop) active.classList.remove('visible');
      active = pop;
      pop.classList.add('visible');
      var rect = mi.getBoundingClientRect();
      var pw = 400, ph = pop.offsetHeight;
      var left = rect.right + 8;
      var top = rect.top;
      if (left + pw > window.innerWidth - 16) {
        left = rect.left - pw - 8;
        if (left < 16) left = 16;
      }
      if (top + ph > window.innerHeight - 16) {
        top = window.innerHeight - ph - 16;
      }
      if (top < 16) top = 16;
      pop.style.left = left + 'px';
      pop.style.top = top + 'px';
    });
    mi.addEventListener('mouseleave', function() {
      var pop = document.getElementById(mi.getAttribute('data-pop'));
      if (!pop) return;
      setTimeout(function() {
        if (!pop.matches(':hover') && !mi.matches(':hover')) {
          pop.classList.remove('visible');
          active = null;
        }
      }, 100);
    });
  });
  document.querySelectorAll('.mpop').forEach(function(pop) {
    pop.addEventListener('mouseleave', function() {
      setTimeout(function() {
        if (!pop.matches(':hover')) {
          var trigger = document.querySelector('[data-pop="' + pop.id + '"]');
          if (!trigger || !trigger.matches(':hover')) {
            pop.classList.remove('visible');
            active = null;
          }
        }
      }, 100);
    });
  });
}

function route() {
  var hash = location.hash || '#/';
  var html;
  if (hash === '#/' || hash === '#' || hash === '') {
    html = renderRoot();
  } else if (hash.indexOf('#/dir/') === 0) {
    html = renderDir(hash.substring(6));
  } else if (hash.indexOf('#/file/') === 0) {
    html = renderFile(hash.substring(7));
  } else {
    html = renderRoot();
  }
  app.innerHTML = html;
  initPopups();
}

window.addEventListener('hashchange', route);
window.nav = nav;
route();
})();
</script>
</body>
</html>
)JS";
  ofs.close();
  if (!ofs) {
    throw IOException(errno, fmt::format("Failed to write '{}': {}",
                                         htmlPath.string(), std::strerror(errno)));
  }
}  // NOLINT(readability/fn_size)

}  // namespace sentinel
