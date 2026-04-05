/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <cstddef>
#include <filesystem>  // NOLINT
#include <string>
#include <tuple>
#include <vector>
#include "sentinel/docGenerator/SrcHtmlGenerator.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

namespace fs = std::filesystem;

SrcHtmlGenerator::SrcHtmlGenerator(const std::string& srcName, bool srcRoot)
    : mSrcRoot(srcRoot), mSrcName(srcName) {
}

SrcHtmlGenerator::SrcHtmlGenerator(const std::string& srcName, bool srcRoot,
                                   const std::filesystem::path& dirPath,
                                   std::size_t killed, std::size_t survived,
                                   std::size_t skipped,
                                   const std::string& skippedDetail,
                                   const std::string& version)
    : mSrcRoot(srcRoot),
      mSrcName(srcName),
      mDirPath(dirPath),
      mKilled(killed),
      mSurvived(survived),
      mSkipped(skipped),
      mSkippedDetail(skippedDetail),
      mVersion(version),
      mHasExtendedData(true) {
}

void SrcHtmlGenerator::pushLine(
    std::size_t curLineNum, const std::string& curClass,
    std::size_t numCurLineMrs, const std::string& curCode,
    const std::vector<std::tuple<int, std::string, std::string,
        std::string, bool, std::string>>& explain) {
  // Determine row class
  std::string rowClassAttr;
  if (curClass == "killed" || curClass == "lk") {
    rowClassAttr = " class=\"lk\"";
  } else if (curClass == "survived" || curClass == "ls") {
    rowClassAttr = " class=\"ls\"";
  } else if (curClass == "uncertain") {
    rowClassAttr = " class=\"ls\"";
  }

  // Build mutation indicator cell
  std::string miCell;
  if (numCurLineMrs > 0) {
    miCell = fmt::format(
        "\n        <td class=\"mi\" data-pop=\"pop-{}\">{}</td>",
        curLineNum, numCurLineMrs);
  } else {
    miCell = "<td class=\"mi\"></td>";
  }

  mLines += fmt::format(
      "      <tr id=\"line-{}\"{}>\n"
      "        <td class=\"ln\">{}</td>{}\n"
      "        <td class=\"cd\"><pre>{}</pre></td>\n"
      "      </tr>\n",
      curLineNum,
      rowClassAttr, curLineNum, miCell, escape(curCode));

  // Build popup for this line if mutations exist
  if (numCurLineMrs > 0 && !explain.empty()) {
    std::string popContent;
    bool first = true;
    for (const auto& entry : explain) {
      if (!first) {
        popContent += "    <hr class=\"mpop__sep\"/>\n";
      }
      first = false;

      const bool killed = std::get<4>(entry);
      const auto& killingTest = std::get<5>(entry);
      const char* badgeClass = killed ? "b-k" : "b-s";
      const char* badgeText = killed ? "Killed" : "Survived";
      const auto& op = std::get<1>(entry);
      const auto& oriCode = std::get<2>(entry);
      const auto& mutCode = std::get<3>(entry);

      const std::string testDisplay = killingTest.empty() ? "none" : killingTest;
      const char* testColor = killed ? "var(--green)" : "var(--text-muted)";

      popContent += fmt::format(
          "    <div class=\"mpop__t\">{} <span class=\"mpop__badge {}\">{}</span></div>\n"
          "    <div class=\"mpop__lbl\">Original</div>\n"
          "    <div class=\"mpop__code\">{}</div>\n"
          "    <div class=\"mpop__lbl\">Mutated</div>\n"
          "    <div class=\"mpop__code\">{}</div>\n"
          "    <div class=\"mpop__lbl\" style=\"margin-top:8px\">Killed by</div>\n"
          "    <div style=\"font-size:.82rem;color:{};margin-top:3px\">{}</div>\n",
          op, badgeClass, badgeText,
          escape(oriCode), escape(mutCode),
          testColor, testDisplay);
    }

    mPopups += fmt::format(
        "  <div id=\"pop-{}\" class=\"mpop\">\n{}"
        "  </div>\n\n",
        curLineNum, popContent);
  }
}

void SrcHtmlGenerator::pushMutation(std::size_t curLineNum, bool killed,
                                    std::size_t count,
                                    const std::string& curKillingTest,
                                    const std::string& curOperator) {
  const char* badgeClass = killed ? "b-k" : "b-s";
  const char* badgeText = killed ? "Killed" : "Survived";
  const std::string testDisplay = curKillingTest.empty() ? "none" : curKillingTest;

  mMutations += fmt::format(
      "      <div class=\"ment\">\n"
      "        <a class=\"ment__ln\" href=\"#line-{}\">{}</a>\n"
      "        <span class=\"ment__op\">{}</span>\n"
      "        <span class=\"ment__kl\">{}</span>\n"
      "        <span class=\"ment__st {}\">{}</span>\n"
      "      </div>\n",
      curLineNum, fmt::format(":{}", curLineNum), curOperator,
      testDisplay, badgeClass, badgeText);
}

void SrcHtmlGenerator::pushMutator(const std::string& mutator) {
  mMutators += fmt::format(
      "        <span class=\"tag\">{}</span>\n", mutator);
}

void SrcHtmlGenerator::pushKillingTest(const std::string& killingTest) {
  mTestList += fmt::format(
      "        <span class=\"tag\">{}</span>\n", killingTest);
}

std::string SrcHtmlGenerator::buildCardsHtml() const {
  const std::size_t valid = mKilled + mSurvived;
  const unsigned int score = valid > 0 ? 100 * mKilled / valid : 0;

  std::string skippedSub;
  if (mSkipped > 0 && !mSkippedDetail.empty()) {
    skippedSub = mSkippedDetail;
  } else if (mSkipped > 0) {
    skippedSub = fmt::format("{} skipped", mSkipped);
  } else {
    skippedSub = "None";
  }

  return fmt::format(
      "  <section class=\"cards cards--4\">\n"
      "    <div class=\"card card--score\">\n"
      "      <div class=\"card__lbl\">Mutation Score</div>\n"
      "      <div class=\"card__val\">{}%</div>\n"
      "      <div class=\"card__sub\">{} / {} valid mutants</div>\n"
      "      <div class=\"mini-bar\">"
      "<div class=\"mini-bar__fill\" style=\"width:{}%\"></div></div>\n"
      "    </div>\n"
      "    <div class=\"card card--killed\">\n"
      "      <div class=\"card__lbl\">Killed</div>\n"
      "      <div class=\"card__val\">{}</div>\n"
      "      <div class=\"card__sub\">Detected by tests</div>\n"
      "    </div>\n"
      "    <div class=\"card card--surv\">\n"
      "      <div class=\"card__lbl\">Survived</div>\n"
      "      <div class=\"card__val\">{}</div>\n"
      "      <div class=\"card__sub\">Not detected</div>\n"
      "    </div>\n"
      "    <div class=\"card card--skip\">\n"
      "      <div class=\"card__lbl\">Skipped</div>\n"
      "      <div class=\"card__val\">{}</div>\n"
      "      <div class=\"card__sub\">{}</div>\n"
      "    </div>\n"
      "  </section>\n",
      score, mKilled, valid, score,
      mKilled, mSurvived, mSkipped, skippedSub);
}

std::string SrcHtmlGenerator::buildBreadcrumbHtml() const {
  std::string dirLink;
  if (!mDirPath.empty() && mDirPath != ".") {
    dirLink = fmt::format(
        "<span class=\"crumb__sep\">/</span>"
        "<a href=\"./index.html\">{}</a>",
        mDirPath.string());
  }

  return fmt::format(
      "  <nav class=\"crumb\">\n"
      "    <a href=\"../../index.html\">Project</a>{}"
      "<span class=\"crumb__sep\">/</span>"
      "<span style=\"color:var(--text)\">{}</span>\n"
      "  </nav>\n",
      dirLink, mSrcName);
}

static const char* kPopupJs = R"JS(<script>
(function() {
  var active = null;
  document.querySelectorAll('.mi[data-pop]').forEach(function(mi) {
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
})();
</script>)JS";

std::string SrcHtmlGenerator::str() const {
  if (!mHasExtendedData) {
    // Legacy output for backward compatibility
    return fmt::format(
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"UTF-8\"/>\n"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/>\n"
        "  <title>{} — Sentinel Report</title>\n"
        "  <link rel=\"stylesheet\" href=\"{}../style.css\"/>\n"
        "</head>\n"
        "<body>\n"
        "<div class=\"wrap\">\n"
        "  <h1>{}</h1>\n"
        "  <table class=\"src-tbl\">\n"
        "{}"
        "  </table>\n"
        "</div>\n"
        "</body>\n"
        "</html>\n",
        mSrcName, mSrcRoot ? "" : "../", mSrcName, mLines);
  }

  const std::string styleRef = mSrcRoot ? "../" : "../../";

  std::string mutatorsPanel;
  if (!mMutators.empty()) {
    mutatorsPanel = fmt::format(
        "    <div class=\"panel\">\n"
        "      <div class=\"panel__t\">Active Mutators</div>\n"
        "      <div class=\"tags\">\n"
        "{}"
        "      </div>\n"
        "    </div>\n",
        mMutators);
  }

  std::string testsPanel;
  if (!mTestList.empty()) {
    testsPanel = fmt::format(
        "    <div class=\"panel\">\n"
        "      <div class=\"panel__t\">Tests Examined</div>\n"
        "      <div class=\"tags\">\n"
        "{}"
        "      </div>\n"
        "    </div>\n",
        mTestList);
  }

  std::string row2Section;
  if (!mutatorsPanel.empty() || !testsPanel.empty()) {
    row2Section = fmt::format(
        "\n  <section class=\"row2\" style=\"margin-top:24px\">\n"
        "{}{}"
        "  </section>\n",
        mutatorsPanel, testsPanel);
  }

  return fmt::format(
R"(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
  <title>{src_name} — Sentinel Report</title>
  <link rel="stylesheet" href="{style_ref}style.css"/>
</head>
<body>
<div class="wrap">

  <header class="hdr">
    <div class="hdr__left">
      <h1>Mutation Testing Report</h1>
    </div>
  </header>

{breadcrumb}
{cards}
  <section>
    <div class="sec-t">Source Code</div>
    <table class="src-tbl">
{lines}    </table>
  </section>

  <!-- Popups — positioned by JS, outside table -->
{popups}  <!-- Mutations list -->
  <section style="margin-top:24px">
    <div class="sec-t">Mutants</div>
    <div class="mlist">
{mutations}    </div>
  </section>
{row2}
  <footer class="ftr">Report generated by <a href="https://github.com/shift-left-test/sentinel">Sentinel</a> {version}</footer>

</div>

{js}
</body>
</html>
)",
      fmt::arg("src_name", mSrcName),
      fmt::arg("style_ref", styleRef),
      fmt::arg("breadcrumb", buildBreadcrumbHtml()),
      fmt::arg("cards", buildCardsHtml()),
      fmt::arg("lines", mLines),
      fmt::arg("popups", mPopups),
      fmt::arg("mutations", mMutations),
      fmt::arg("row2", row2Section),
      fmt::arg("version", mVersion.empty() ? "" : "v" + mVersion),
      fmt::arg("js", kPopupJs));
}

std::string SrcHtmlGenerator::escape(const std::string& original) const {
  std::string result;
  result.reserve(original.size());
  for (char c : original) {
    switch (c) {
      case '&': result += "&amp;"; break;
      case '<': result += "&lt;"; break;
      case '>': result += "&gt;"; break;
      case '"': result += "&quot;"; break;
      case '\'': result += "&apos;"; break;
      default: result += c; break;
    }
  }
  return result;
}

}  // namespace sentinel
