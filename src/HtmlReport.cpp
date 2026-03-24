/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "sentinel/HtmlReport.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/docGenerator/CssGenerator.hpp"
#include "sentinel/docGenerator/EmptyIndexHtmlGenerator.hpp"
#include "sentinel/docGenerator/IndexHtmlGenerator.hpp"
#include "sentinel/docGenerator/SrcHtmlGenerator.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/operators/MutationOperatorExpansion.hpp"
#include "sentinel/util/io.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

namespace fs = std::filesystem;

HtmlReport::HtmlReport(const MutationSummary& summary) : Report(summary) {
}

void HtmlReport::save(const std::filesystem::path& dirPath) {
  Logger::info("Make HTML Report");

  io::ensureDirectoryExists(dirPath);

  std::ofstream ofs(dirPath / "style.css", std::ofstream::out);
  CssGenerator cg;
  ofs << cg.str();
  ofs.close();

  if (mSummary.totNumberOfMutation == 0) {
    std::ofstream ofs2(dirPath / "index.html", std::ofstream::out);
    ofs2 << EmptyIndexHtmlGenerator().str();
    ofs2.close();
    return;
  }

  makeIndexHtml(mSummary.totNumberOfMutation, mSummary.totNumberOfDetectedMutation, true, "", dirPath);

  for (const auto& p : mSummary.groupByDirPath) {
    makeIndexHtml(mSummary.totNumberOfMutation, mSummary.totNumberOfDetectedMutation, false, p.first, dirPath);
  }

  for (const auto& p : mSummary.groupByPath) {
    makeSourceHtml(p.second.results, p.first, dirPath);
  }
}

void HtmlReport::makeIndexHtml(std::size_t totNumberOfMutation, std::size_t totNumberOfDetectedMutation, bool root,
                               const std::filesystem::path& currentDirPath, const std::filesystem::path& outputDir) {
  std::size_t sizeOfTargetFiles = 0;
  std::size_t numerator = 0;
  std::size_t denominator = 0;
  if (root) {
    sizeOfTargetFiles = mSummary.groupByPath.size();
    numerator = totNumberOfDetectedMutation;
    denominator = totNumberOfMutation;
  } else {
    sizeOfTargetFiles = mSummary.groupByDirPath.at(currentDirPath).fileCount;
    numerator = mSummary.groupByDirPath.at(currentDirPath).detected;
    denominator = mSummary.groupByDirPath.at(currentDirPath).total;
  }
  unsigned int cov = 100 * numerator / denominator;

  IndexHtmlGenerator ihg(root, currentDirPath, sizeOfTargetFiles, cov, numerator, denominator);

  if (root) {
    for (const auto& p : mSummary.groupByDirPath) {
      std::size_t numOfFiles = p.second.fileCount;
      std::size_t subDetected = p.second.detected;
      std::size_t subMut = p.second.total;
      unsigned int subCov = 100 * subDetected / subMut;
      ihg.pushItemToTable(p.first, subCov, subDetected, subMut, numOfFiles);
    }
  } else {
    for (const auto& p : mSummary.groupByPath) {
      fs::path curDirpath = p.first.parent_path();
      if (currentDirPath != curDirpath) {
        continue;
      }

      std::size_t subDetected = p.second.detected;
      std::size_t subMut = p.second.total;
      auto subCov = 100 * subDetected / subMut;

      ihg.pushItemToTable(p.first.filename(), subCov, subDetected, subMut, -1);
    }
  }

  auto contents = ihg.str();

  fs::path fileName = "index.html";
  if (!root) {
    fs::path relDir = string::replaceAll(currentDirPath.string(), "/", ".");
    fileName = fs::path("srcDir") / relDir / "index.html";
    auto newDir = outputDir / "srcDir" / relDir;
    io::ensureDirectoryExists(newDir);
  }
  std::ofstream ofs(outputDir / fileName, std::ofstream::out);
  ofs << contents;
  ofs.close();
  Logger::info("Save to {}", outputDir / fileName);
}

void HtmlReport::makeSourceHtml(const std::vector<const MutationResult*>& mrs, const std::filesystem::path& srcPath,
                                const std::filesystem::path& outputDir) {
  auto absSrcPath = mSummary.sourcePath / srcPath;
  if (!fs::exists(absSrcPath)) {
    throw InvalidArgumentException(fmt::format("Source doesn't exist: {0}", absSrcPath.string()));
  }

  std::string srcName = absSrcPath.filename().string();

  std::map<std::size_t, std::shared_ptr<std::vector<const MutationResult*>>> groupByLine;
  std::set<std::string> uniqueKillingTest;
  std::set<std::string> uniqueMutator;

  std::size_t maxLineNum = 0;
  for (const MutationResult* mr : mrs) {
    auto tmpvector = string::split(mr->getKillingTest(), ", ");
    for (const auto& ts : tmpvector) {
      if (!ts.empty()) {
        uniqueKillingTest.insert(ts);
      }
    }
    uniqueMutator.insert(mr->getMutant().getOperator());

    std::size_t curLineNum = mr->getMutant().getFirst().line;
    if (curLineNum == 0) {
      throw InvalidArgumentException(fmt::format("Mutation at line number 0"));
    }
    if (curLineNum > maxLineNum) {
      maxLineNum = curLineNum;
    }
    if (groupByLine.empty() || groupByLine.count(curLineNum) == 0) {
      groupByLine.emplace(curLineNum, std::make_shared<std::vector<const MutationResult*>>());
    }
    groupByLine[curLineNum]->push_back(mr);
  }

  std::ifstream tf(absSrcPath.string());
  std::stringstream buffer;
  buffer << tf.rdbuf();
  std::string srcContents = buffer.str();
  tf.close();

  auto srcLineByLine = string::split(srcContents, '\n');

  if (srcLineByLine.empty() || srcLineByLine.size() < maxLineNum) {
    throw InvalidArgumentException(fmt::format("Src file's line num({0}) is smaller than mutation' line num({1})",
                                               srcLineByLine.size(), maxLineNum));
  }

  SrcHtmlGenerator shg(srcName, srcPath.parent_path().empty());

  for (auto it = srcLineByLine.begin(); it != srcLineByLine.end(); ++it) {
    auto curLineNum = std::distance(srcLineByLine.begin(), it) + 1;
    std::shared_ptr<std::vector<const MutationResult*>> curLineMrs(nullptr);
    if (groupByLine.count(curLineNum) != 0) {
      curLineMrs = groupByLine[curLineNum];
    }

    const char* curClass = "";
    if (curLineMrs != nullptr) {
      bool killed = false;
      bool survived = false;
      for (const auto& mr : *curLineMrs) {
        if (mr->getDetected()) {
          killed = true;
        } else {
          survived = true;
        }
      }
      if (killed && survived) {
        curClass = "uncertain";
      } else if (killed) {
        curClass = "killed";
      } else if (survived) {
        curClass = "survived";
      }
    }
    size_t numCurLineMrs = curLineMrs != nullptr ? curLineMrs->size() : 0;

    std::vector<std::tuple<int, std::string, std::string, std::string, bool>> lineExplainVec;

    if (curLineMrs != nullptr) {
      std::size_t count = 0;
      for (const auto& mr : *curLineMrs) {
        count += 1;
        std::string oriCode;
        std::string mutatedCodeHead;
        std::string mutatedCodeTail;
        auto first = mr->getMutant().getFirst();
        auto last = mr->getMutant().getLast();
        for (int i = first.line; i <= static_cast<int>(last.line); i++) {
          std::string curLineContent = srcLineByLine[i - 1];
          if (!oriCode.empty()) {
            oriCode += "\n";
          }
          if (i == first.line) {
            mutatedCodeHead = curLineContent.substr(0, first.column - 1);
          }
          if (i == static_cast<int>(last.line)) {
            mutatedCodeTail = curLineContent.substr(last.column - 1, std::string::npos);
          }
          oriCode.append(curLineContent);
        }
        std::string mutatedCode;
        mutatedCode.append(mutatedCodeHead);
        mutatedCode.append(mr->getMutant().getToken());
        mutatedCode.append(mutatedCodeTail);
        lineExplainVec.emplace_back(count, MutationOperatorToExpansion(mr->getMutant().getOperator()), oriCode,
                                    mutatedCode, mr->getDetected());
      }
    }
    shg.pushLine(curLineNum, curClass, numCurLineMrs, *it, lineExplainVec);
  }

  for (const auto& t : groupByLine) {
    std::size_t count = 0;
    for (const auto& mr : *t.second) {
      count += 1;
      shg.pushMutation(t.first, mr->getDetected(), count, mr->getKillingTest(),
                       MutationOperatorToExpansion(mr->getMutant().getOperator()));
    }
  }

  for (const auto& ts : uniqueMutator) {
    shg.pushMutator(MutationOperatorToExpansion(ts));
  }

  for (const auto& ts : uniqueKillingTest) {
    shg.pushKillingTest(ts);
  }

  auto contents = shg.str();

  fs::path relDir = string::replaceAll(srcPath.parent_path().string(), "/", ".");
  fs::path fileName = outputDir / "srcDir" / relDir / fmt::format("{0}.html", srcName);

  std::ofstream ofs(fileName.string(), std::ofstream::out);
  ofs << contents;
  ofs.close();

  Logger::info("Save to {}", fileName);
}

}  // namespace sentinel
