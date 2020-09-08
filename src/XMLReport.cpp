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
#include <cstdio>
#include <ctime>
#include <string>
#include "sentinel/XMLReport.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/util/filesystem.hpp"


namespace sentinel {

XMLReport::XMLReport(const std::string& resultsPath) : mResults(resultsPath) {
  mResults.load();
}

void XMLReport::save(const std::string& path) {
  if (util::filesystem::exists(path) && !util::filesystem::isDirectory(path)) {
      throw IOException(EINVAL);
  }

  std::time_t rawtime;
  std::tm* timeinfo;
  char buffer[80];

  std::time(&rawtime);
  timeinfo = std::localtime(&rawtime);
  std::strftime(static_cast<char*> (buffer), 80, "%Y%m%d%H%M", timeinfo);

  auto dirPath = util::filesystem::join(path, buffer);
  util::filesystem::createDirectory(dirPath);

  auto xmlPath = util::filesystem::join(dirPath, "mutations.xml");

  auto doc = new tinyxml2::XMLDocument();
  tinyxml2::XMLDeclaration* pDecl = doc->NewDeclaration();
  doc->InsertFirstChild(pDecl);

  tinyxml2::XMLElement* pMutations = doc->NewElement("mutations");

  for (const auto& r : mResults) {
    tinyxml2::XMLElement* pMutation = doc->NewElement("mutation");
    pMutation->SetAttribute("detected", r.getDetected());

    addChildToParent(doc, pMutation, "sourceFile",
        util::filesystem::filename(r.getPath()));
    addChildToParent(doc, pMutation, "sourceFilePath", r.getPath());
    addChildToParent(doc, pMutation, "mutatedClass", r.getMutatedClass());
    addChildToParent(doc, pMutation, "mutatedMethod", r.getMutatedMethod());
    addChildToParent(doc, pMutation, "methodDescription",
        r.getMethodDescription());
    addChildToParent(doc, pMutation, "lineNumber",
        std::to_string(r.getLineNum()));
    addChildToParent(doc, pMutation, "mutator", r.getMutator());
    addChildToParent(doc, pMutation, "killingTest", r.getKillingTest());

    pMutations->InsertEndChild(pMutation);
  }

  doc->InsertEndChild(pMutations);
  doc->SaveFile(xmlPath.c_str());
  delete doc;
}

void XMLReport::addChildToParent(tinyxml2::XMLDocument* d,
    tinyxml2::XMLElement* p, const std::string& childName,
    const std::string& childText) {
    tinyxml2::XMLElement* pChild = d->NewElement(childName.c_str());
    pChild->SetText(childText.c_str());
    p->InsertEndChild(pChild);
}


}  // namespace sentinel
