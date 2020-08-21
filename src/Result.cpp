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

#include <tinyxml2/tinyxml2.h>
#include <algorithm>
#include <vector>
#include <string>
#include "sentinel/exceptions/XMLException.hpp"
#include "sentinel/Result.hpp"
#include "sentinel/util/filesystem.hpp"


namespace sentinel {

Result::Result(const std::string& path) {
  mPassedTC.clear();
  auto xmlFiles = util::filesystem::findFilesInDirUsingExt(path, {"xml"});
  for (const std::string& xmlPath : xmlFiles) {
    tinyxml2::XMLDocument doc;
    auto errcode = doc.LoadFile(xmlPath.c_str());
    if (errcode != 0) {
      throw sentinel::XMLException(errcode);
    }
    tinyxml2::XMLElement *pRoot = doc.FirstChildElement("testsuites");
    for (tinyxml2::XMLElement *p = pRoot->FirstChildElement("testsuite") ;
        p != nullptr ; p = p->NextSiblingElement("testsuite")) {
      for (tinyxml2::XMLElement *q = p->FirstChildElement("testcase") ;
          q != nullptr ; q = q->NextSiblingElement("testcase")) {
        if (std::string(q->Attribute("status")) == std::string("run")  &&
            q->FirstChildElement("failure") == nullptr) {
          std::string className = std::string(q->Attribute("classname"));
          std::string caseName = std::string(q->Attribute("name"));
          mPassedTC.push_back(className.append(".").append(caseName));
        }
      }
    }
  }
  if (!mPassedTC.empty()) {
    std::sort(mPassedTC.begin(), mPassedTC.end());
  }
}

bool Result::kill(const Result& original, const Result& mutated) {
  bool kill = false;
  for (const std::string &tc : original.mPassedTC) {
    if (std::lower_bound(mutated.mPassedTC.begin(),
        mutated.mPassedTC.end(), tc) == mutated.mPassedTC.end()) {
      kill = true;
      break;
    }
  }
  return kill;
}

}  // namespace sentinel
