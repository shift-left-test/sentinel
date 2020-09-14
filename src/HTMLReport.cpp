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
#include <ctime>
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
#include "sentinel/util/filesystem.hpp"
#include "sentinel/util/string.hpp"


namespace sentinel {

HTMLReport::HTMLReport(const std::string& resultsPath,
    const std::string& sourcePath) :
    mSourcePath(sourcePath), mResults(resultsPath) {
  mResults.load();
  if (!util::filesystem::exists(sourcePath) ||
      !util::filesystem::isDirectory(sourcePath)) {
    throw InvalidArgumentException(fmt::format("sourcePath doesn't exist({0})",
        sourcePath));
  }
}

void HTMLReport::save(const std::string& path) {
  if (util::filesystem::exists(path)) {
    if (!util::filesystem::isDirectory(path)) {
      throw InvalidArgumentException(fmt::format("path isn't direcotry({0})",
        path));
    }
  } else {
    util::filesystem::createDirectory(path);
  }

  std::time_t rawtime;
  std::tm* timeinfo;
  char buffer[80];

  rawtime = mResults.getLastModified();
  if (rawtime == -1) {
    std::time(&rawtime);
  }
  timeinfo = std::localtime(&rawtime);
  std::strftime(static_cast<char*> (buffer), 80, "%Y%m%d%H%M", timeinfo);

  auto dirPath = util::filesystem::join(path, buffer);
  if (util::filesystem::exists(dirPath)) {
    if (!util::filesystem::isDirectory(dirPath)) {
      throw InvalidArgumentException(fmt::format("path isn't direcotry({0})",
        dirPath));
    }
  } else {
    util::filesystem::createDirectory(dirPath);
  }

  // tuple: MutationResult, # of mutation, # of detected mutation
  //        (# of files in Dir)
  std::map<std::string,
      std::tuple<std::vector<const MutationResult*>*, int, int, int>* >
      groupByDirPath;
  std::map<std::string,
      std::tuple<std::vector<const MutationResult*>*, int, int>* > groupByPath;
  int totNumberOfMutation = mResults.size();
  int totNumberOfDetectedMutation = 0;

  for ( const MutationResult& mr : mResults ) {
    auto mrPath = mr.getPath();
    std::string curDirname = util::filesystem::dirname(mrPath);
    curDirname = util::string::replaceAll(curDirname, "/", ".");

    if (groupByDirPath.empty() || groupByDirPath.count(curDirname) == 0) {
      groupByDirPath.emplace(curDirname,
          new std::tuple<std::vector<const MutationResult*>*, int, int, int>(
          new std::vector<const MutationResult*>(), 0, 0, 0));
    }
    std::get<0>(*groupByDirPath[curDirname])->push_back(&mr);
    std::get<1>(*groupByDirPath[curDirname]) += 1;

    if (groupByPath.empty() ||
        groupByPath.count(mrPath) == 0) {
      groupByPath.emplace(mrPath,
          new std::tuple<std::vector<const MutationResult*>*, int, int>(
          new std::vector<const MutationResult*>(), 0, 0));
    }
    std::get<0>(*groupByPath[mrPath])->push_back(&mr);
    std::get<1>(*groupByPath[mrPath]) += 1;

    if (mr.getDetected()) {
      std::get<2>(*groupByDirPath[curDirname]) += 1;
      std::get<2>(*groupByPath[mrPath]) += 1;
      totNumberOfDetectedMutation += 1;
    }
  }

  for ( auto const& p : groupByDirPath ) {
    std::set<std::string> tmpSet;
    for (const MutationResult* mr : *(std::get<0>(*p.second))) {
      tmpSet.insert(mr->getPath());
    }
    std::get<3>(*p.second) = tmpSet.size();
  }

  auto outputPath = util::filesystem::join(path, buffer);

  std::ofstream ofs(util::filesystem::join(outputPath, "style.css"),
      std::ofstream::out);
  ofs << styleCssContent;
  ofs.close();

  makeIndexHtml(&groupByDirPath, &groupByPath, totNumberOfMutation,
      totNumberOfDetectedMutation, true, "", outputPath);

  for ( auto const& p : groupByDirPath ) {
    makeIndexHtml(&groupByDirPath, &groupByPath, totNumberOfMutation,
        totNumberOfDetectedMutation, false, p.first, outputPath);
  }

  for ( auto const& p : groupByPath ) {
    makeSourceHtml(std::get<0>(*p.second), p.first, outputPath);
  }

  for ( auto const& p : groupByDirPath ) {
    delete std::get<0>(*p.second);
    delete p.second;
  }

  for ( auto const& p : groupByPath ) {
    delete std::get<0>(*p.second);
    delete p.second;
  }
}

tinyxml2::XMLElement* HTMLReport::insertNewNode(tinyxml2::XMLDocument* doc,
    tinyxml2::XMLNode* parent, const char* elementName,
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
        std::tuple<std::vector<const MutationResult*>*, int, int, int>* >*
        pGroupByDirPath,
      std::map<std::string,
        std::tuple<std::vector<const MutationResult*>*, int, int>* >*
        pGroupByPath,
      int totNumberOfMutation, int totNumberOfDetectedMutation, bool root,
      const std::string& currentDirPath, const std::string& outputDir
    ) {
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

  int sizeOfTargetFiles = 0;
  int numerator = 0;
  int denominator = 0;
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
      fmt::format("{0}/{1}", numerator, denominator).c_str());
  pDiv3->SetAttribute("class", "coverage_legend");

  insertNewNode(doc, pBody, "h3", fmt::format("Breakdown by {0}",
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
    for ( auto const& p : *pGroupByDirPath ) {
      auto pTr4 = insertNewNode(doc, pTbody2, "tr");
      int numOfFiles = std::get<3>(*p.second);
      int numerator2 = std::get<2>(*p.second);
      int denominator2 = std::get<1>(*p.second);

      auto pTd0 = insertNewNode(doc, pTr4, "td");
      auto pA = insertNewNode(doc, pTd0, "a", p.first.c_str());
      pA->SetAttribute("href", fmt::format("./{0}/index.html",
          p.first).c_str());

      insertNewNode(doc, pTr4, "td", std::to_string(numOfFiles).c_str());

      auto cov2 = static_cast<int>
          (static_cast<double>(numerator2) /
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
    for ( auto const& p : *pGroupByPath ) {
      {
        std::string curDirname = util::filesystem::dirname(p.first);
        curDirname = util::string::replaceAll(curDirname, "/", ".");

        if (currentDirPath != curDirname) {
          continue;
        }

        auto pTr5 = insertNewNode(doc, pTbody2, "tr");

        int numerator3 = std::get<2>(*p.second);
        int denominator3 = std::get<1>(*p.second);

        std::string curfilename = util::filesystem::filename(p.first);

        auto pTd0 = insertNewNode(doc, pTr5, "td");

        auto pA = insertNewNode(doc, pTd0, "a", curfilename.c_str());
        pA->SetAttribute("href", fmt::format("./{0}.html",
            curfilename).c_str());

        auto cov3 = static_cast<int>
            (static_cast<double>(numerator3) /
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
    fileName = util::filesystem::join(currentDirPath, "index.html");
    std::string newDir = util::filesystem::join(outputDir, currentDirPath);
    if (util::filesystem::exists(newDir)) {
      if (!util::filesystem::isDirectory(newDir)) {
        throw InvalidArgumentException(fmt::format("path isn't direcotry({0})",
          newDir));
      }
    } else {
      util::filesystem::createDirectory(newDir);
    }
  }
  doc->SaveFile(util::filesystem::join(outputDir, fileName).c_str());
  delete doc;
}


void HTMLReport::makeSourceHtml(
    std::vector<const MutationResult*>* MRs, const std::string& srcPath,
    const std::string& outputDir) {

  if (!util::filesystem::exists(srcPath)) {
    throw InvalidArgumentException(
        fmt::format("Source doesn't exists: {0}", srcPath));
  }

  std::string srcName = util::filesystem::filename(srcPath);

  std::map <int, std::vector<const MutationResult*>*> groupByLine;
  std::set <std::string> uniqueKillingTest;
  std::set <std::string> uniqueMutator;

  int maxLineNum = 0;
  for ( const MutationResult* mr : *MRs ) {
    auto tmpvector = util::string::splitByStringDelimiter(
        mr->getKillingTest(), ", ");
    for (auto const& ts : tmpvector) {
      if (!ts.empty()) {
        uniqueKillingTest.insert(ts);
      }
    }
    uniqueMutator.insert(mr->getMutator());

    int curLineNum = mr->getLineNum();
    if (curLineNum == 0) {
      throw InvalidArgumentException(
          fmt::format("Muation at line number 0(Mutable DB Index{0})",
          mr->getIndexOfMutableDB()));
    }
    if (curLineNum > maxLineNum) {
      maxLineNum = curLineNum;
    }
    if (groupByLine.empty() || groupByLine.count(curLineNum) == 0) {
      groupByLine.emplace(curLineNum, new std::vector<const MutationResult*>());
    }
    groupByLine[curLineNum]->push_back(mr);
  }

  std::ifstream tf(srcPath);
  std::stringstream buffer;
  buffer << tf.rdbuf();
  std::string srcContents = buffer.str();
  tf.close();

  auto srcLineByLine = util::string::split(srcContents, '\n');

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

  for ( auto it = srcLineByLine.begin() ; it != srcLineByLine.end() ; ++it ) {
    auto curLineNum = std::distance(srcLineByLine.begin(), it) + 1;
    std::vector<const MutationResult*>* curLineMrs = nullptr;
    if ( groupByLine.count(curLineNum) != 0 ) {
      curLineMrs = groupByLine[curLineNum];
    }

    const char* curClass = "";
    if (curLineMrs != nullptr) {
      bool killed = false;
      bool survived = false;
      for (auto const& mr : *curLineMrs) {
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
      int count = 0;
      for (auto const& mr : *curLineMrs) {
        count += 1;
        insertNewNode(doc, pSpanMutator, "b",
            fmt::format("{0}. {1} -> {2}", count, mr->getMutator(),
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

  for ( auto const& t : groupByLine ) {
    int count = 0;
    for ( auto const& mr : *t.second ) {
      count += 1;
      auto pTr3 = insertNewNode(doc, pTable, "tr");
      auto pTd3 = insertNewNode(doc, pTr3, "td");

      auto pA = insertNewNode(doc, pTd3, "a",
          std::to_string(mr->getLineNum()).c_str());
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
      insertNewNode(doc, pP, "span", fmt::format("{0} -> {1}", mr->getMutator(),
          mr->getDetected() ? "KILLED" : "NO_COVERAGE").c_str());
    }
  }

  insertNewNode(doc, pBody, "h2", "Active mutators");
  auto pUl = insertNewNode(doc, pBody, "ul");

  for (auto const& ts : uniqueMutator) {
    auto pTmp = insertNewNode(doc, pUl, "li", ts.c_str());
    pTmp->SetAttribute("class", "mutator");
  }

  insertNewNode(doc, pBody, "h2", "Tests examined");
  auto pUl2 = insertNewNode(doc, pBody, "ul");

  for (auto const& ts : uniqueKillingTest) {
    insertNewNode(doc, pUl2, "li", ts.c_str());
  }

  insertNewNode(doc, pBody, "hr");
  auto ph5 = insertNewNode(doc, pBody, "h5", "Report generated by ");
  auto pA = insertNewNode(doc, ph5, "a", "Sentinel");
  pA->SetAttribute("href", "http://mod.lge.com/hub/yocto/addons/sentinel");

  std::string fileName = util::filesystem::join(outputDir,
      util::string::replaceAll(util::filesystem::dirname(srcPath), "/", "."),
      fmt::format("{0}.html", srcName));
  doc->SaveFile(fileName.c_str());

  delete doc;

  for ( auto const& t : groupByLine ) {
    delete t.second;
  }
}

}  // namespace sentinel
