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
#include <tinyxml2/tinyxml2.h>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/HTMLReport.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/util/os.hpp"
#include "sentinel/util/string.hpp"


namespace sentinel {

HTMLReport::HTMLReport(const std::string& resultsPath,
                       const std::string& sourcePath) :
    Report(resultsPath, sourcePath) {
}

void HTMLReport::save(const std::string& dirPath) {
  if (os::path::exists(dirPath)) {
    if (!os::path::isDirectory(dirPath)) {
      throw InvalidArgumentException(fmt::format("dirPath isn't direcotry({0})",
                                                 dirPath));
    }
  } else {
    os::createDirectories(dirPath);
  }

  std::ofstream ofs(os::path::join(dirPath, "style.css"),
                    std::ofstream::out);
  ofs << styleCssContent;
  ofs.close();

  makeIndexHtml(&groupByDirPath, &groupByPath, totNumberOfMutation,
                totNumberOfDetectedMutation, true, "", dirPath);

  for (const auto& p : groupByDirPath) {
    makeIndexHtml(&groupByDirPath, &groupByPath, totNumberOfMutation,
                  totNumberOfDetectedMutation, false, p.first, dirPath);
  }

  for (const auto& p : groupByPath) {
    makeSourceHtml(std::get<0>(*p.second), p.first, dirPath);
  }
}

tinyxml2::XMLElement* HTMLReport::insertNewNode(
    tinyxml2::XMLDocument* doc,
    tinyxml2::XMLNode* parent,
    const char* elementName,
    const char* elementText = nullptr) {
  auto child = doc -> NewElement(elementName);
  if (elementText != nullptr) {
    child->SetText(elementText);
  }
  parent->InsertEndChild(child);
  return child;
}

void HTMLReport::makeIndexHtml(
    std::map<std::string,
    std::tuple<std::vector<const MutationResult*>*,
    std::size_t, std::size_t, std::size_t>* >*
    pGroupByDirPath,
    std::map<std::string,
    std::tuple<std::vector<const MutationResult*>*,
    std::size_t, std::size_t>* >*
    pGroupByPath,
    std::size_t totNumberOfMutation, std::size_t totNumberOfDetectedMutation,
    bool root, const std::string& currentDirPath,
    const std::string& outputDir) {
  auto doc = new tinyxml2::XMLDocument();

  auto pDOCTYPE = doc->NewUnknown("DOCTYPE html");
  doc->InsertEndChild(pDOCTYPE);

  auto pHtml = insertNewNode(doc, doc, "html");

  auto pHead = insertNewNode(doc, pHtml, "head");

  auto pLink = insertNewNode(doc, pHead, "link");
  pLink->SetAttribute("rel", "stylesheet");
  pLink->SetAttribute("type", "text/css");
  pLink->SetAttribute("href", root ? "style.css" : "../style.css");

  auto pBody = insertNewNode(doc, pHtml, "body");
  insertNewNode(doc, pBody, "h1",
                "Sentinel Mutation Coverage Report");

  if (root) {
    insertNewNode(doc, pBody, "h3", "Proejct Summary");
  } else {
    insertNewNode(doc, pBody, "h2", "Directory Summary");
    insertNewNode(doc, pBody, "h3", currentDirPath.c_str());
  }

  auto pTable = insertNewNode(doc, pBody, "table");
  auto pThead = insertNewNode(doc, pTable, "thead");
  auto pTr = insertNewNode(doc, pThead, "tr");
  insertNewNode(doc, pTr, "th", "Number of Files");
  insertNewNode(doc, pTr, "th", "Mutation Coverage");
  auto pTbody = insertNewNode(doc, pTable, "tbody");
  auto pTr2 = insertNewNode(doc, pTbody, "tr");

  std::size_t sizeOfTargetFiles = 0;
  std::size_t numerator = 0;
  std::size_t denominator = 0;
  if (root) {
    sizeOfTargetFiles = pGroupByPath->size();
    numerator = totNumberOfDetectedMutation;
    denominator = totNumberOfMutation;
  } else {
    sizeOfTargetFiles = std::get<3>(*(*pGroupByDirPath)[currentDirPath]);
    numerator = std::get<2>(*(*pGroupByDirPath)[currentDirPath]);
    denominator = std::get<1>(*(*pGroupByDirPath)[currentDirPath]);
  }
  auto cov = static_cast<int> (static_cast<double>(numerator) /
                               denominator * 100);

  insertNewNode(doc, pTr2, "td", std::to_string(sizeOfTargetFiles).c_str());
  auto pTd = insertNewNode(
      doc, pTr2, "td", fmt::format("{0}% ", cov).c_str());

  auto pDiv1 = insertNewNode(doc, pTd, "div");
  pDiv1->SetAttribute("class", "coverage_bar");
  auto pDiv2 = insertNewNode(doc, pDiv1, "div");
  pDiv2->SetAttribute("class",
                      fmt::format("coverage_complete width-{0}", cov).c_str());
  auto pDiv3 = insertNewNode(doc, pDiv2, "div",
                             fmt::format("{0}/{1}",
                                         numerator, denominator).c_str());
  pDiv3->SetAttribute("class", "coverage_legend");

  insertNewNode(doc, pBody, "h3",
                fmt::format("Breakdown by {0}",
                            root ? "Directory" : "File").c_str());

  auto pTable2 = insertNewNode(doc, pBody, "table");

  auto pThead2 = insertNewNode(doc, pTable2, "thead");
  auto pTr3 = insertNewNode(doc, pThead2, "tr");
  insertNewNode(doc, pTr3, "th", "Name");
  if (root) {
    insertNewNode(doc, pTr3, "th", "Number of Files");
  }
  insertNewNode(doc, pTr3, "th", "Mutation Coverage");

  auto pTbody2 = insertNewNode(doc, pTable2, "tbody");

  if (root) {
    for (const auto& p : *pGroupByDirPath) {
      auto pTr4 = insertNewNode(doc, pTbody2, "tr");
      std::size_t numOfFiles = std::get<3>(*p.second);
      std::size_t numerator2 = std::get<2>(*p.second);
      std::size_t denominator2 = std::get<1>(*p.second);

      auto pTd0 = insertNewNode(doc, pTr4, "td");
      auto pA = insertNewNode(doc, pTd0, "a", p.first.c_str());
      pA->SetAttribute("href", fmt::format("./{0}/index.html",
          p.first).c_str());

      insertNewNode(doc, pTr4, "td", std::to_string(numOfFiles).c_str());

      auto cov2 = static_cast<int>(static_cast<double>(numerator2) /
                                   denominator2 * 100);
      auto pTd2 = insertNewNode(doc, pTr4, "td");

      auto pDiv4 = insertNewNode(doc, pTd2, "div",
          fmt::format("{0}% ", cov2).c_str());
      pDiv4->SetAttribute("class", "coverage_percentage");

      auto pDiv5 = insertNewNode(doc, pTd2, "div");
      pDiv5->SetAttribute("class", "coverage_bar");

      auto pDiv6 = insertNewNode(doc, pDiv5, "div");
      pDiv6->SetAttribute("class",
            fmt::format("coverage_complete width-{0}", cov2).c_str());

      auto pDiv7 = insertNewNode(doc, pDiv6, "div", fmt::format("{0}/{1}",
              numerator2, denominator2).c_str());
      pDiv7->SetAttribute("class", "coverage_legend");
    }
  } else {
    for (const auto& p : *pGroupByPath) {
      {
        std::string curDirname = os::path::dirname(p.first);
        curDirname = string::replaceAll(curDirname, "/", ".");

        if (currentDirPath != curDirname) {
          continue;
        }

        auto pTr5 = insertNewNode(doc, pTbody2, "tr");

        std::size_t numerator3 = std::get<2>(*p.second);
        std::size_t denominator3 = std::get<1>(*p.second);

        std::string curfilename = os::path::filename(p.first);

        auto pTd0 = insertNewNode(doc, pTr5, "td");

        auto pA = insertNewNode(doc, pTd0, "a", curfilename.c_str());
        pA->SetAttribute("href", fmt::format("./{0}.html",
            curfilename).c_str());

        auto cov3 = static_cast<int>(static_cast<double>(numerator3) /
                                     denominator3 * 100);

        auto pTd2 = insertNewNode(doc, pTr5, "td");

        auto pDiv8 = insertNewNode(doc, pTd2, "div",
            fmt::format("{0}% ", cov3).c_str());
        pDiv8->SetAttribute("class", "coverage_percentage");

        auto pDiv9 = insertNewNode(doc, pTd2, "div");
        pDiv9->SetAttribute("class", "coverage_bar");

        auto pDiv10 = insertNewNode(doc, pDiv9, "div");
        pDiv10->SetAttribute("class",
            fmt::format("coverage_complete width-{0}", cov3).c_str());

        auto pDiv11 = insertNewNode(doc, pDiv10, "div",
            fmt::format("{0}/{1}", numerator3, denominator3).c_str());
        pDiv11->SetAttribute("class", "coverage_legend");
      }
    }
  }

  insertNewNode(doc, pBody, "hr");
  auto ph5 = insertNewNode(doc, pBody, "h5", "Report generated by ");
  auto pA = insertNewNode(doc, ph5, "a", "Sentinel");
  pA->SetAttribute("href", "http://mod.lge.com/hub/yocto/addons/sentinel");

  std::string fileName = "index.html";
  if (!root) {
    fileName = os::path::join(currentDirPath, "index.html");
    std::string newDir = os::path::join(outputDir, currentDirPath);
    if (os::path::exists(newDir)) {
      if (!os::path::isDirectory(newDir)) {
        throw InvalidArgumentException(fmt::format("path isn't direcotry({0})",
                                                   newDir));
      }
    } else {
      os::createDirectories(newDir);
    }
  }
  doc->SaveFile(os::path::join(outputDir, fileName).c_str());
  delete doc;
}

void HTMLReport::makeSourceHtml(
    std::vector<const MutationResult*>* MRs,
    const std::string& srcPath,
    const std::string& outputDir) {
  auto absSrcPath = os::path::join(mSourcePath, srcPath);
  if (!os::path::exists(absSrcPath)) {
    throw InvalidArgumentException(
        fmt::format("Source doesn't exists: {0}", absSrcPath));
  }

  std::string srcName = os::path::filename(absSrcPath);

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
    uniqueMutator.insert(mr->getMutable().getOperator());

    std::size_t curLineNum = mr->getMutable().getFirst().line;
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

  auto doc = new tinyxml2::XMLDocument();

  auto pDOCTYPE = doc->NewUnknown("DOCTYPE html");
  doc->InsertEndChild(pDOCTYPE);

  auto pHtml = insertNewNode(doc, doc, "html");

  auto pHead = insertNewNode(doc, pHtml, "head");

  auto pLink = insertNewNode(doc, pHead, "link");
  pLink->SetAttribute("rel", "stylesheet");
  pLink->SetAttribute("type", "text/css");
  pLink->SetAttribute("href", "../style.css");

  auto pBody = insertNewNode(doc, pHtml, "body");


  insertNewNode(doc, pBody, "h1", srcName.c_str());

  auto pTable = insertNewNode(doc, pBody, "table");
  pTable->SetAttribute("class", "src");

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

    auto pTr0 = insertNewNode(doc, pTable, "tr");
    auto pTd0 = insertNewNode(doc, pTr0, "td",
        fmt::format("{0}", curLineNum).c_str());
    pTd0->SetAttribute("class", "na");
    auto pA = insertNewNode(doc, pTd0, "a");
    pA->SetAttribute("name",
        fmt::format("sentinel.report.html.SourceFile@{0}_{1}",
        srcName, curLineNum).c_str());
    auto pTd1 = insertNewNode(doc, pTr0, "td");
    pTd1->SetAttribute("class", curClass);

    auto pSpan0 = insertNewNode(doc, pTd1, "span");
    pSpan0->SetAttribute("class", "pop");
    auto pA1 = insertNewNode(doc, pSpan0, "a", curLineMrs != nullptr ?
        std::to_string(curLineMrs->size()).c_str() : "");
    pA1->SetAttribute("href", fmt::format(
          "#group.sentinel.report.html.SourceFile@{0}_{1}",
          srcName, curLineNum).c_str());
    auto pSpanMutator = insertNewNode(doc, pSpan0, "span");

    if (curLineMrs != nullptr) {
      std::size_t count = 0;
      for (const auto& mr : *curLineMrs) {
        count += 1;
        insertNewNode(doc, pSpanMutator, "b",
            fmt::format("{0}. {1} -> {2}", count,
            mr->getMutable().getOperator(),
            mr->getDetected() ? "KILLED" : "NO_COVERAGE").c_str());
        insertNewNode(doc, pSpanMutator, "br");
      }
    }

    auto pTd2 = insertNewNode(doc, pTr0, "td");
    pTd2->SetAttribute("class", curClass);

    auto pSpan1 = insertNewNode(doc, pTd2, "span");
    pSpan1->SetAttribute("class", curClass);
    insertNewNode(doc, pSpan1, "pre", (*it).c_str());
  }

  auto pTr2 = insertNewNode(doc, pTable, "tr");
  insertNewNode(doc, pTr2, "td");
  insertNewNode(doc, pTr2, "td");
  auto pTd2 = insertNewNode(doc, pTr2, "td");
  insertNewNode(doc, pTd2, "h2", "Mutations");

  for (const auto& t : groupByLine) {
    std::size_t count = 0;
    for (const auto& mr : *t.second) {
      count += 1;
      auto pTr3 = insertNewNode(doc, pTable, "tr");
      auto pTd3 = insertNewNode(doc, pTr3, "td");

      auto pA = insertNewNode(doc, pTd3, "a",
          std::to_string(mr->getMutable().getFirst().line).c_str());
      pA->SetAttribute("href",
          fmt::format("#sentinel.report.html.SourceFile@{0}_{1}",
          srcName, t.first).c_str());
      insertNewNode(doc, pTr3, "td");
      auto pTd4 = insertNewNode(doc, pTr3, "td");
      auto pA2 = insertNewNode(doc, pTd4, "a");
      pA2->SetAttribute("name", fmt::format(
            "group.sentinel.report.html.SourceFile@{0}_{1}",
            srcName, t.first).c_str());
      auto pP = insertNewNode(doc, pTd4, "a");
      pP->SetAttribute("class", mr->getDetected() ? "KILLED" : "NO_COVERAGE");
      auto pSpan2 = insertNewNode(doc, pP, "span", fmt::format("{0}.",
          count).c_str());
      pSpan2->SetAttribute("class", "pop");
      auto pSpan3 = insertNewNode(doc, pSpan2, "span");
      insertNewNode(doc, pSpan3, "b", fmt::format("{0} ", count).c_str());
      insertNewNode(doc, pSpan3, "br");
      insertNewNode(doc, pSpan3, "b",
          fmt::format("Location: line num {0}", t.first).c_str());
      insertNewNode(doc, pSpan3, "br");
      auto curKillingTest = mr->getKillingTest();
      insertNewNode(doc, pSpan3, "b", fmt::format("Killed by : {0}",
          curKillingTest.empty() ? "none" : curKillingTest.c_str()).c_str());
      insertNewNode(doc, pP, "span", fmt::format("{0} -> {1}",
          mr->getMutable().getOperator(),
          mr->getDetected() ? "KILLED" : "NO_COVERAGE").c_str());
    }
  }

  insertNewNode(doc, pBody, "h2", "Active mutators");
  auto pUl = insertNewNode(doc, pBody, "ul");

  for (const auto& ts : uniqueMutator) {
    auto pTmp = insertNewNode(doc, pUl, "li", ts.c_str());
    pTmp->SetAttribute("class", "mutator");
  }

  insertNewNode(doc, pBody, "h2", "Tests examined");
  auto pUl2 = insertNewNode(doc, pBody, "ul");

  for (const auto& ts : uniqueKillingTest) {
    insertNewNode(doc, pUl2, "li", ts.c_str());
  }

  insertNewNode(doc, pBody, "hr");
  auto ph5 = insertNewNode(doc, pBody, "h5", "Report generated by ");
  auto pA = insertNewNode(doc, ph5, "a", "Sentinel");
  pA->SetAttribute("href", "http://mod.lge.com/hub/yocto/addons/sentinel");

  std::string fileName = os::path::join(outputDir,
      string::replaceAll(os::path::dirname(srcPath), "/", "."),
      fmt::format("{0}.html", srcName));
  doc->SaveFile(fileName.c_str());

  delete doc;

  for (const auto& t : groupByLine) {
    delete t.second;
  }
}

}  // namespace sentinel
