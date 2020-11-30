 /*
  MIT License

  Copyright (c) 2020 Sung Gon Kim

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <fmt/core.h>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/docGenerator/CSSGenerator.hpp"
#include "sentinel/docGenerator/IndexHTMLGenerator.hpp"
#include "sentinel/docGenerator/SrcHTMLGenerator.hpp"
#include "sentinel/HTMLReport.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/util/string.hpp"


namespace sentinel {

HTMLReport::HTMLReport(const MutationResults& results,
                       const std::string& sourcePath) :
    Report(results, sourcePath) {
}

HTMLReport::HTMLReport(const std::string& resultsPath,
                       const std::string& sourcePath) :
    Report(resultsPath, sourcePath) {
}

void HTMLReport::save(const std::experimental::filesystem::path& dirPath) {
  namespace fs = std::experimental::filesystem;

  if (totNumberOfMutation == 0) {
    return;
  }

  if (fs::exists(dirPath)) {
    if (!fs::is_directory(dirPath)) {
      throw InvalidArgumentException(fmt::format("dirPath isn't direcotry({0})",
                                                 dirPath.string()));
    }
  } else {
    fs::create_directories(dirPath);
  }

  std::ofstream ofs(dirPath / "style.css", std::ofstream::out);
  CSSGenerator cg;
  ofs << cg.str();
  ofs.close();

  makeIndexHtml(totNumberOfMutation, totNumberOfDetectedMutation, true, "",
                dirPath);

  for (const auto& p : groupByDirPath) {
    makeIndexHtml(totNumberOfMutation, totNumberOfDetectedMutation, false,
                  p.first, dirPath);
  }

  for (const auto& p : groupByPath) {
    makeSourceHtml(std::get<0>(*p.second), p.first, dirPath);
  }
}

void HTMLReport::makeIndexHtml(
    std::size_t totNumberOfMutation, std::size_t totNumberOfDetectedMutation,
    bool root, const std::experimental::filesystem::path& currentDirPath,
    const std::experimental::filesystem::path& outputDir) {
  namespace fs = std::experimental::filesystem;

  std::size_t sizeOfTargetFiles = 0;
  std::size_t numerator = 0;
  std::size_t denominator = 0;
  if (root) {
    sizeOfTargetFiles = groupByPath.size();
    numerator = totNumberOfDetectedMutation;
    denominator = totNumberOfMutation;
  } else {
    sizeOfTargetFiles = std::get<3>(*groupByDirPath[currentDirPath]);
    numerator = std::get<2>(*groupByDirPath[currentDirPath]);
    denominator = std::get<1>(*groupByDirPath[currentDirPath]);
  }
  unsigned int cov = 100 * numerator / denominator;

  IndexHTMLGenerator ihg(root, currentDirPath, sizeOfTargetFiles,
                         cov, numerator, denominator);

  if (root) {
    for (const auto& p : groupByDirPath) {
      std::size_t numOfFiles = std::get<3>(*p.second);
      std::size_t subDetected = std::get<2>(*p.second);
      std::size_t subMut = std::get<1>(*p.second);
      unsigned int subCov = 100 * subDetected / subMut;
      ihg.pushItemToTable(p.first, subCov, subDetected, subMut, numOfFiles);
    }
  } else {
    for (const auto& p : groupByPath) {
      std::string curDirname = p.first.parent_path();
      curDirname = string::replaceAll(curDirname, "/", ".");

      if (currentDirPath != curDirname) {
        continue;
      }

      std::size_t subDetected = std::get<2>(*p.second);
      std::size_t subMut = std::get<1>(*p.second);
      auto subCov = 100 * subDetected / subMut;

      ihg.pushItemToTable(p.first.filename(),
                          subCov, subDetected, subMut, -1);
    }
  }

  auto contents = ihg.str();

  std::string fileName = "index.html";
  if (!root) {
    fileName = "srcDir" / currentDirPath / "index.html";
    auto newDir = outputDir / "srcDir" / currentDirPath;
    if (fs::exists(newDir)) {
      if (!fs::is_directory(newDir)) {
        throw InvalidArgumentException(fmt::format("path isn't direcotry({0})",
                                                   newDir.string()));
      }
    } else {
      fs::create_directories(newDir);
    }
  }
  std::ofstream ofs(outputDir / fileName, std::ofstream::out);
  ofs << contents;
  ofs.close();
}

void HTMLReport::makeSourceHtml(
    std::vector<const MutationResult*>* MRs,
    const std::experimental::filesystem::path& srcPath,
    const std::experimental::filesystem::path& outputDir) {
  namespace fs = std::experimental::filesystem;

  auto absSrcPath = mSourcePath / srcPath;
  if (!fs::exists(absSrcPath)) {
    throw InvalidArgumentException(
        fmt::format("Source doesn't exists: {0}", absSrcPath.string()));
  }

  std::string srcName = absSrcPath.filename();

  std::map<std::size_t, std::vector<const MutationResult*>*> groupByLine;
  std::set<std::string> uniqueKillingTest;
  std::set<std::string> uniqueMutator;

  std::size_t maxLineNum = 0;
  for (const MutationResult* mr : *MRs) {
    auto tmpvector = string::split(
        mr->getKillingTest(), ", ");
    for (const auto& ts : tmpvector) {
      if (!ts.empty()) {
        uniqueKillingTest.insert(ts);
      }
    }
    uniqueMutator.insert(mr->getMutant().getOperator());

    std::size_t curLineNum = mr->getMutant().getFirst().line;
    if (curLineNum == 0) {
      throw InvalidArgumentException(
          fmt::format("Muation at line number 0"));
    }
    if (curLineNum > maxLineNum) {
      maxLineNum = curLineNum;
    }
    if (groupByLine.empty() || groupByLine.count(curLineNum) == 0) {
      groupByLine.emplace(curLineNum, new std::vector<const MutationResult*>());
    }
    groupByLine[curLineNum]->push_back(mr);
  }

  std::ifstream tf(absSrcPath);
  std::stringstream buffer;
  buffer << tf.rdbuf();
  std::string srcContents = buffer.str();
  tf.close();

  auto srcLineByLine = string::split(srcContents, '\n');

  if (srcLineByLine.empty() || srcLineByLine.size() < maxLineNum) {
    throw InvalidArgumentException(
        fmt::format(
            "Src file's line num({0}) is smaller than mutation' line num({1})",
            srcLineByLine.size(), maxLineNum));
  }

  SrcHTMLGenerator shg(srcName, srcPath.parent_path().empty());

  for (auto it = srcLineByLine.begin() ; it != srcLineByLine.end() ; ++it) {
    auto curLineNum = std::distance(srcLineByLine.begin(), it) + 1;
    std::vector<const MutationResult*>* curLineMrs = nullptr;
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
    size_t numCurLineMrs = curLineMrs != nullptr ?
        curLineMrs->size() : 0;

    std::vector<std::tuple<int, std::string, bool>> lineExplainVec;

    if (curLineMrs != nullptr) {
      std::size_t count = 0;
      for (const auto& mr : *curLineMrs) {
        count += 1;
        lineExplainVec.emplace_back(count, mr->getMutant().getOperator(),
                                    mr->getDetected());
      }
    }
    shg.pushLine(curLineNum, curClass, numCurLineMrs, *it, lineExplainVec);
  }

  for (const auto& t : groupByLine) {
    std::size_t count = 0;
    for (const auto& mr : *t.second) {
      count += 1;
      shg.pushMutation(t.first, mr->getDetected(), count,
          mr->getKillingTest(),
          mr->getMutant().getOperator());
    }
  }

  for (const auto& ts : uniqueMutator) {
    shg.pushMutator(ts);
  }

  for (const auto& ts : uniqueKillingTest) {
    shg.pushKillingTest(ts);
  }

  auto contents = shg.str();

  std::string fileName = outputDir / "srcDir" /
      string::replaceAll(srcPath.parent_path(), "/", ".") /
      fmt::format("{0}.html", srcName);

  std::ofstream ofs(fileName, std::ofstream::out);
  ofs << contents;
  ofs.close();

  for (const auto& t : groupByLine) {
    delete t.second;
  }
}

}  // namespace sentinel
