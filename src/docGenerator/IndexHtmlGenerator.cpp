/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <array>
#include <cmath>
#include <filesystem>  // NOLINT
#include <map>
#include <string>
#include <vector>
#include "sentinel/docGenerator/IndexHtmlGenerator.hpp"
#include "sentinel/operators/MutationOperatorExpansion.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

namespace fs = std::filesystem;

static constexpr double kPi = 3.14159265358979323846;
static constexpr double kDonutRadius = 54.0;
static constexpr double kCircumference = 2.0 * kPi * kDonutRadius;
static constexpr unsigned int kHighThreshold = 70;
static constexpr unsigned int kMidThreshold = 40;

IndexHtmlGenerator::IndexHtmlGenerator(
    bool root, const std::filesystem::path& dirName,
    std::size_t sizeOfTargetFiles, unsigned int score,
    std::size_t numerator, std::size_t denominator,
    std::size_t skipped, const std::string& skippedDetail)
    : mRoot(root),
      mDirName(dirName),
      mSizeOfTargetFiles(sizeOfTargetFiles),
      mScore(score),
      mNumerator(numerator),
      mDenominator(denominator),
      mSkipped(skipped),
      mSkippedDetail(skippedDetail) {
}

IndexHtmlGenerator::IndexHtmlGenerator(
    bool root, const std::filesystem::path& dirName,
    std::size_t sizeOfTargetFiles, unsigned int score,
    std::size_t numerator, std::size_t denominator,
    const MutationSummary& summary, const Config& config,
    const std::string& timestamp, const std::string& version)
    : mRoot(root),
      mDirName(dirName),
      mSizeOfTargetFiles(sizeOfTargetFiles),
      mScore(score),
      mNumerator(numerator),
      mDenominator(denominator),
      mSkipped(summary.totNumberOfBuildFailure +
               summary.totNumberOfRuntimeError +
               summary.totNumberOfTimeout),
      mSkippedDetail(formatSkippedDetail(
          summary.totNumberOfTimeout,
          summary.totNumberOfBuildFailure,
          summary.totNumberOfRuntimeError)),
      mSummary(&summary),
      mConfig(&config),
      mTimestamp(timestamp),
      mVersion(version) {
}

void IndexHtmlGenerator::pushItemToTable(
    const std::string& subName, int subScore,
    std::size_t subNumerator, std::size_t subDenominator,
    std::size_t numOfFiles) {
  std::string subPath;
  if (mRoot) {
    fs::path relDir = string::replaceAll(subName, "/", ".");
    subPath = fmt::format("./srcDir/{0}index.html",
                          relDir.empty() ? "" : relDir.string() + "/");
  } else {
    subPath = fmt::format("./{0}.html", subName);
  }

  const auto* covClass = coverageClass(
      static_cast<unsigned int>(subScore));
  const auto* fillClass = coverageFillClass(
      static_cast<unsigned int>(subScore));
  const std::string displayName = subName.empty() ? "." : subName;

  if (mRoot) {
    mTableItem += fmt::format(
R"(        <tr>
          <td><a href="{}">{}</a></td>
          <td style="text-align:center">{}</td>
          <td><div class="cov-cell"><span class="cov-pct {}">{}%</span><div class="cov-bar"><div class="cov-bar__fill {}" style="width:{}%"></div></div><span class="cov-ratio">{}/{}</span></div></td>
        </tr>
)",
        subPath, displayName, numOfFiles,
        covClass, subScore, fillClass, subScore,
        subNumerator, subDenominator);
  } else {
    mTableItem += fmt::format(
R"(        <tr>
          <td><a href="{}">{}</a></td>
          <td><div class="cov-cell"><span class="cov-pct {}">{}%</span><div class="cov-bar"><div class="cov-bar__fill {}" style="width:{}%"></div></div><span class="cov-ratio">{}/{}</span></div></td>
        </tr>
)",
        subPath, displayName,
        covClass, subScore, fillClass, subScore,
        subNumerator, subDenominator);
  }
}

std::string IndexHtmlGenerator::str() const {
  const std::string stylePath = mRoot ? "" :
      (mDirName.empty() ? "../" : "../../");

  const std::string title = mRoot ?
      "Sentinel &mdash; Mutation Testing Report" :
      fmt::format("{} &mdash; Sentinel Report",
                  mDirName.empty() ? "." : mDirName.filename().string());

  std::string body;
  body += buildCardsHtml();

  if (mRoot && mSummary != nullptr) {
    body += buildRootPanelsHtml();
    body += buildConfigHtml();
  }

  body += buildTableHtml();

  const std::string headerRight = (mRoot && !mTimestamp.empty()) ?
      fmt::format(
R"(    <div class="hdr__right">
      <span class="badge">Generated: {}</span>
    </div>)", mTimestamp) : "";

  const std::string breadcrumb = (!mRoot) ? buildBreadcrumbHtml() : "";

  const std::string footerVersion = mRoot && !mVersion.empty() ?
      " v" + mVersion : "";

  return fmt::format(
R"(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
  <title>{title}</title>
  <link rel="stylesheet" href="{style_path}style.css"/>
</head>
<body>
<div class="wrap">

  <header class="hdr">
    <div class="hdr__left">
      <h1>Mutation Testing Report</h1>
    </div>
{header_right}
  </header>

{breadcrumb}{body}
  <footer class="ftr">Report generated by <a href="https://github.com/shift-left-test/sentinel">Sentinel</a>{footer_version}</footer>

</div>
</body>
</html>
)",
      fmt::arg("title", title),
      fmt::arg("style_path", stylePath),
      fmt::arg("header_right", headerRight),
      fmt::arg("breadcrumb", breadcrumb),
      fmt::arg("body", body),
      fmt::arg("footer_version", footerVersion));
}

std::string IndexHtmlGenerator::buildCardsHtml() const {
  const std::size_t survived = mDenominator - mNumerator;

  return fmt::format(
R"(  <section class="cards cards--4">
    <div class="card card--score">
      <div class="card__lbl">Mutation Score</div>
      <div class="card__val">{}%</div>
      <div class="card__sub">{} / {} valid mutants</div>
      <div class="mini-bar"><div class="mini-bar__fill" style="width:{}%"></div></div>
    </div>
    <div class="card card--killed">
      <div class="card__lbl">Killed</div>
      <div class="card__val">{}</div>
      <div class="card__sub">Detected by tests</div>
    </div>
    <div class="card card--surv">
      <div class="card__lbl">Survived</div>
      <div class="card__val">{}</div>
      <div class="card__sub">Not detected</div>
    </div>
    <div class="card card--skip">
      <div class="card__lbl">Skipped</div>
      <div class="card__val">{}</div>
      <div class="card__sub">{}</div>
    </div>
  </section>

)",
      mScore,
      mNumerator, mDenominator,
      mScore,
      mNumerator,
      survived,
      mSkipped, mSkippedDetail);
}

/// @cond
std::string IndexHtmlGenerator::buildRootPanelsHtml() const {
  if (mSummary == nullptr) {
    return "";
  }

  const auto& summary = *mSummary;
  const std::size_t killed = mNumerator;
  const std::size_t survived = mDenominator - mNumerator;
  const std::size_t timeout = summary.totNumberOfTimeout;
  const std::size_t buildFailure = summary.totNumberOfBuildFailure;
  const std::size_t runtimeError = summary.totNumberOfRuntimeError;
  const std::size_t total = killed + survived + timeout +
                            buildFailure + runtimeError;

  // --- Mutants donut ---
  std::string mutantDonutSegments;
  std::string mutantLegend;
  if (total > 0) {
    const double killedArc =
        static_cast<double>(killed) / static_cast<double>(total) * kCircumference;
    const double survivedArc =
        static_cast<double>(survived) / static_cast<double>(total) * kCircumference;
    const std::size_t skippedCount = timeout + buildFailure + runtimeError;
    const double skippedArc =
        static_cast<double>(skippedCount) / static_cast<double>(total) * kCircumference;

    // stroke-dashoffset: first segment starts at top (84.8 = circumference/4)
    const double startOffset = kCircumference / 4.0;

    mutantDonutSegments += fmt::format(
R"(          <circle cx="70" cy="70" r="54" fill="none" stroke="#0f8a5f" stroke-width="15"
             stroke-dasharray="{:.1f} {:.1f}" stroke-dashoffset="{:.1f}"
             style="transform:rotate(-90deg);transform-origin:center;"/>
)",
        killedArc, kCircumference - killedArc, startOffset);

    mutantDonutSegments += fmt::format(
R"(          <circle cx="70" cy="70" r="54" fill="none" stroke="#d44030" stroke-width="15"
             stroke-dasharray="{:.1f} {:.1f}" stroke-dashoffset="{:.1f}"
             style="transform:rotate(-90deg);transform-origin:center;"/>
)",
        survivedArc, kCircumference - survivedArc,
        startOffset - killedArc);

    mutantDonutSegments += fmt::format(
R"(          <circle cx="70" cy="70" r="54" fill="none" stroke="#94a0b0" stroke-width="15"
             stroke-dasharray="{:.1f} {:.1f}" stroke-dashoffset="{:.1f}"
             style="transform:rotate(-90deg);transform-origin:center;"/>
)",
        skippedArc, kCircumference - skippedArc,
        startOffset - killedArc - survivedArc);

    mutantLegend = fmt::format(
R"(          <div class="legend-i"><span class="legend-dot" style="background:#0f8a5f"></span>Killed<span class="n">{}</span></div>
          <div class="legend-i"><span class="legend-dot" style="background:#d44030"></span>Survived<span class="n">{}</span></div>)",
        killed, survived);
    if (timeout > 0) {
      mutantLegend += fmt::format(
          "\n          <div class=\"legend-i\"><span class=\"legend-dot\""
          " style=\"background:#94a0b0\"></span>Timeout<span class=\"n\">"
          "{}</span></div>", timeout);
    }
    if (buildFailure > 0) {
      mutantLegend += fmt::format(
          "\n          <div class=\"legend-i\"><span class=\"legend-dot\""
          " style=\"background:#94a0b0\"></span>Build Failure<span class=\"n\">"
          "{}</span></div>", buildFailure);
    }
    if (runtimeError > 0) {
      mutantLegend += fmt::format(
          "\n          <div class=\"legend-i\"><span class=\"legend-dot\""
          " style=\"background:#94a0b0\"></span>Runtime Error<span class=\"n\">"
          "{}</span></div>", runtimeError);
    }
  }

  // --- By Operator bars ---
  const std::vector<std::string> kOperatorOrder =
      {"AOR", "BOR", "LCR", "ROR", "SDL", "SOR", "UOI"};

  // Per-operator counts: [0]=killed, [1]=survived, [2]=skipped
  std::map<std::string, std::array<std::size_t, 3>> opMap;
  for (const auto& mr : summary.results) {
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

  std::string operatorBars;
  for (const auto& op : kOperatorOrder) {
    auto it = opMap.find(op);
    if (it == opMap.end()) {
      continue;
    }
    const auto opKilled = it->second[0];
    const auto opSurvived = it->second[1];
    const auto opSkipped = it->second[2];
    const std::size_t opTotal = opKilled + opSurvived + opSkipped;
    if (opTotal == 0) {
      continue;
    }

    // Bar widths as percentages
    const double kPct = 100.0 * static_cast<double>(opKilled) /
                        static_cast<double>(opTotal);
    const double sPct = 100.0 * static_cast<double>(opSurvived) /
                        static_cast<double>(opTotal);
    const double xPct = 100.0 * static_cast<double>(opSkipped) /
                        static_cast<double>(opTotal);

    // Build count description
    std::string cnt = fmt::format("{} killed", opKilled);
    if (opSurvived > 0) {
      cnt += fmt::format(" &middot; {} survived", opSurvived);
    }
    if (opSkipped > 0) {
      cnt += fmt::format(" &middot; {} skipped", opSkipped);
    }

    std::string bars;
    if (opKilled > 0) {
      bars += fmt::format(
          "<div class=\"bar-k\" style=\"width:{:.1f}%\"></div>", kPct);
    }
    if (opSurvived > 0) {
      bars += fmt::format(
          "<div class=\"bar-s\" style=\"width:{:.1f}%\"></div>", sPct);
    }
    if (opSkipped > 0) {
      bars += fmt::format(
          "<div class=\"bar-x\" style=\"width:{:.1f}%\"></div>", xPct);
    }

    operatorBars += fmt::format(
R"(        <div>
          <div class="bar-hdr"><span class="bar-name">{}</span><span class="bar-cnt">{}</span></div>
          <div class="bar-track">{}</div>
        </div>
)",
        MutationOperatorToExpansion(op), cnt, bars);
  }

  // --- Duration donut ---
  const double totalSecs = summary.totalBuildSecs + summary.totalTestSecs;
  std::string durationDonutSegments;
  std::string durationLegend;
  if (totalSecs > 0.0) {
    const double buildArc = summary.totalBuildSecs / totalSecs * kCircumference;
    const double testArc = summary.totalTestSecs / totalSecs * kCircumference;
    const double startOffset = kCircumference / 4.0;

    durationDonutSegments += fmt::format(
R"(          <circle cx="70" cy="70" r="54" fill="none" stroke="#036098" stroke-width="15"
             stroke-dasharray="{:.1f} {:.1f}" stroke-dashoffset="{:.1f}"
             style="transform:rotate(-90deg);transform-origin:center;"/>
)",
        buildArc, kCircumference - buildArc, startOffset);

    durationDonutSegments += fmt::format(
R"(          <circle cx="70" cy="70" r="54" fill="none" stroke="#48b0e0" stroke-width="15"
             stroke-dasharray="{:.1f} {:.1f}" stroke-dashoffset="{:.1f}"
             style="transform:rotate(-90deg);transform-origin:center;"/>
)",
        testArc, kCircumference - testArc, startOffset - buildArc);

    durationLegend = fmt::format(
R"(          <div class="legend-i"><span class="legend-dot" style="background:#036098"></span>Build<span class="n">{}</span></div>
          <div class="legend-i"><span class="legend-dot" style="background:#48b0e0"></span>Test<span class="n">{}</span></div>)",
        formatDuration(summary.totalBuildSecs),
        formatDuration(summary.totalTestSecs));
  }

  // --- By State bars ---
  const std::vector<std::pair<std::string, MutationState>> kStateOrder = {
      {"Killed", MutationState::KILLED},
      {"Survived", MutationState::SURVIVED},
      {"Timeout", MutationState::TIMEOUT},
      {"Build Failure", MutationState::BUILD_FAILURE},
      {"Runtime Error", MutationState::RUNTIME_ERROR},
  };

  std::string stateBars;
  for (const auto& [label, state] : kStateOrder) {
    auto it = summary.timeByState.find(state);
    if (it == summary.timeByState.end() || it->second.count == 0) {
      continue;
    }
    const auto& timing = it->second;
    const double stateTotalSecs = timing.buildSecs + timing.testSecs;
    const int pct = (totalSecs > 0.0) ?
        static_cast<int>(std::round(stateTotalSecs / totalSecs * 100.0)) : 0;

    const std::string mutantWord = (timing.count == 1) ? "mutant" : "mutants";
    const std::string cnt = fmt::format(
        "{}% &middot; {} ({} / {}) &middot; {} {}",
        pct, formatDuration(stateTotalSecs),
        formatDuration(timing.buildSecs), formatDuration(timing.testSecs),
        timing.count, mutantWord);

    const double buildBarPct = (totalSecs > 0.0) ?
        timing.buildSecs / totalSecs * 100.0 : 0.0;
    const double testBarPct = (totalSecs > 0.0) ?
        timing.testSecs / totalSecs * 100.0 : 0.0;

    std::string bars;
    if (timing.buildSecs > 0.0) {
      bars += fmt::format(
          "<div class=\"bar-build\" style=\"width:{:.1f}%\"></div>",
          buildBarPct);
    }
    if (timing.testSecs > 0.0) {
      bars += fmt::format(
          "<div class=\"bar-test\" style=\"width:{:.1f}%\"></div>",
          testBarPct);
    }

    stateBars += fmt::format(
R"(        <div>
          <div class="bar-hdr"><span class="bar-name">{}</span><span class="bar-cnt">{}</span></div>
          <div class="bar-track">{}</div>
        </div>
)",
        label, cnt, bars);
  }

  // Assemble the full two-column panel section
  return fmt::format(
R"HTML(  <!-- LEFT: Mutants donut + By Operator | RIGHT: Duration donut + By State -->
  <section class="row2">

    <!-- LEFT -->
    <div class="panel">
      <div class="panel__t">Mutants</div>
      <div class="donut-wrap">
        <svg class="donut-svg" viewBox="0 0 140 140">
          <circle cx="70" cy="70" r="54" fill="none" stroke="#e8ecf1" stroke-width="15"/>
{mutant_segments}          <text x="70" y="66" text-anchor="middle" fill="#002030" font-family="system-ui,sans-serif" font-size="22" font-weight="800">{total}</text>
          <text x="70" y="80" text-anchor="middle" fill="#607080" font-family="system-ui,sans-serif" font-size="10" font-weight="600">TOTAL</text>
        </svg>
        <div class="legend">
{mutant_legend}
        </div>
      </div>

      <div class="panel__t" style="margin-top:24px">By Operator</div>
      <div class="bars">
{operator_bars}      </div>
    </div>

    <!-- RIGHT -->
    <div class="panel">
      <div class="panel__t">Duration</div>
      <div class="donut-wrap">
        <svg class="donut-svg" viewBox="0 0 140 140">
          <circle cx="70" cy="70" r="54" fill="none" stroke="#e8ecf1" stroke-width="15"/>
{duration_segments}          <text x="70" y="66" text-anchor="middle" fill="#002030" font-family="system-ui,sans-serif" font-size="18" font-weight="800">{duration_total}</text>
          <text x="70" y="80" text-anchor="middle" fill="#607080" font-family="system-ui,sans-serif" font-size="10" font-weight="600">TOTAL</text>
        </svg>
        <div class="legend">
{duration_legend}
        </div>
      </div>

      <div class="panel__t" style="margin-top:24px">By State</div>
      <div class="bars">
{state_bars}      </div>
    </div>

  </section>

)HTML",
      fmt::arg("mutant_segments", mutantDonutSegments),
      fmt::arg("total", total),
      fmt::arg("mutant_legend", mutantLegend),
      fmt::arg("operator_bars", operatorBars),
      fmt::arg("duration_segments", durationDonutSegments),
      fmt::arg("duration_total", formatDuration(totalSecs)),
      fmt::arg("duration_legend", durationLegend),
      fmt::arg("state_bars", stateBars));
}
/// @endcond

std::string IndexHtmlGenerator::buildConfigHtml() const {
  if (mConfig == nullptr) {
    return "";
  }
  const auto& cfg = *mConfig;

  std::string items;
  auto addItem = [&items](const std::string& key, const std::string& val) {
    items += fmt::format(
        "      <div class=\"cfg-item\"><span class=\"cfg-k\">{}</span>"
        "<span class=\"cfg-v\">{}</span></div>\n",
        key, val);
  };

  if (!cfg.sourceDir.empty()) {
    addItem("sourceDir", cfg.sourceDir.string());
  }
  if (!cfg.buildCmd.empty()) {
    addItem("buildCmd", cfg.buildCmd);
  }
  if (!cfg.testCmd.empty()) {
    addItem("testCmd", cfg.testCmd);
  }
  addItem("scope", scopeToString(cfg.scope));
  addItem("generator", generatorToString(cfg.generator));
  if (!cfg.operators.empty()) {
    addItem("operators", string::join(", ", cfg.operators));
  }
  if (cfg.limit > 0) {
    addItem("limit", std::to_string(cfg.limit));
  }
  if (cfg.seed.has_value()) {
    addItem("seed", std::to_string(cfg.seed.value()));
  }
  if (cfg.threshold.has_value()) {
    addItem("threshold",
            fmt::format("{}%", static_cast<int>(cfg.threshold.value())));
  }
  if (cfg.timeout.has_value()) {
    addItem("timeout", fmt::format("{}s", cfg.timeout.value()));
  }
  if (cfg.partition.has_value()) {
    addItem("partition", cfg.partition.value());
  }

  // Use string concatenation to avoid escaping braces in onclick handler
  std::string html;
  html += "  <!-- CONFIG -->\n";
  html += "  <button class=\"cfg-btn\" aria-expanded=\"false\" "
          "onclick=\"this.setAttribute('aria-expanded',"
          "this.getAttribute('aria-expanded')==='true'?'false':'true');"
          "this.nextElementSibling.classList.toggle('open')\">\n";
  html += "    <span>Run Configuration</span>\n";
  html += "    <span class=\"cfg-btn__arr\">&#9660;</span>\n";
  html += "  </button>\n";
  html += "  <div class=\"cfg-body\">\n";
  html += "    <div class=\"cfg-grid\">\n";
  html += items;
  html += "    </div>\n";
  html += "  </div>\n\n";
  return html;
}

std::string IndexHtmlGenerator::buildTableHtml() const {
  const std::string tableTitle = "Breakdown by File";

  if (mRoot) {
    return fmt::format(
R"(  <!-- DIRECTORY TABLE -->
  <section class="tbl-sec">
    <div class="sec-t">{}</div>
    <table class="dtbl">
      <thead><tr><th>Name</th><th style="width:70px;text-align:center">Files</th><th style="width:300px">Mutation Score</th></tr></thead>
      <tbody>
{}      </tbody>
    </table>
  </section>

)",
        tableTitle, mTableItem);
  }

  return fmt::format(
R"(  <section class="tbl-sec">
    <div class="sec-t">{}</div>
    <table class="dtbl">
      <thead><tr><th>Name</th><th style="width:300px">Mutation Score</th></tr></thead>
      <tbody>
{}      </tbody>
    </table>
  </section>

)",
      tableTitle, mTableItem);
}

std::string IndexHtmlGenerator::buildBreadcrumbHtml() const {
  const std::string stylePath = mDirName.empty() ? "../" : "../../";
  const std::string displayName = mDirName.empty() ? "." :
      mDirName.string();
  return fmt::format(
      "  <nav class=\"crumb\">\n"
      "    <a href=\"{}index.html\">Project</a>"
      "<span class=\"crumb__sep\">/</span>"
      "<span style=\"color:var(--text)\">{}</span>\n"
      "  </nav>\n\n",
      stylePath, displayName);
}

std::string IndexHtmlGenerator::formatDuration(double secs) {
  if (secs < 60.0) {
    return fmt::format("{:.0f}s", secs);
  }
  auto mins = static_cast<int>(secs) / 60;
  auto remainSecs = static_cast<int>(secs) % 60;
  if (remainSecs == 0) {
    return fmt::format("{}m", mins);
  }
  return fmt::format("{}m {}s", mins, remainSecs);
}

std::string IndexHtmlGenerator::formatSkippedDetail(
    std::size_t timeout, std::size_t buildFailure, std::size_t runtimeError) {
  std::string detail;
  if (timeout > 0) {
    detail += fmt::format("{} timeout", timeout);
  }
  if (buildFailure > 0) {
    if (!detail.empty()) {
      detail += " &middot; ";
    }
    detail += fmt::format("{} build failure", buildFailure);
  }
  if (runtimeError > 0) {
    if (!detail.empty()) {
      detail += " &middot; ";
    }
    detail += fmt::format("{} runtime error", runtimeError);
  }
  return detail;
}

const char* IndexHtmlGenerator::coverageClass(unsigned int cov) {
  if (cov >= kHighThreshold) {
    return "c-hi";
  }
  if (cov >= kMidThreshold) {
    return "c-mid";
  }
  return "c-lo";
}

const char* IndexHtmlGenerator::coverageFillClass(unsigned int cov) {
  if (cov >= kHighThreshold) {
    return "f-hi";
  }
  if (cov >= kMidThreshold) {
    return "f-mid";
  }
  return "f-lo";
}

}  // namespace sentinel
