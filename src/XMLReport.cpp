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
#include <string>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/util/os.hpp"
#include "sentinel/XMLReport.hpp"


namespace sentinel {

XMLReport::XMLReport(const MutationResults& results,
                     const std::string& sourcePath) :
    Report(results, sourcePath) {
}

XMLReport::XMLReport(const std::string& resultsPath,
                     const std::string& sourcePath) :
    Report(resultsPath, sourcePath) {
}

void XMLReport::save(const fs::path& dirPath) {
  if (fs::exists(dirPath)) {
    if (!fs::is_directory(dirPath)) {
      throw InvalidArgumentException(fmt::format("dirPath isn't directory({0})",
                                                 dirPath.string()));
    }
  } else {
    fs::create_directories(dirPath);
  }

  auto xmlPath = dirPath / "mutations.xml";

  auto doc = new tinyxml2::XMLDocument();
  tinyxml2::XMLDeclaration* pDecl = doc->NewDeclaration();
  doc->InsertFirstChild(pDecl);

  tinyxml2::XMLElement* pMutations = doc->NewElement("mutations");

  for (const auto& r : mResults) {
    tinyxml2::XMLElement* pMutation = doc->NewElement("mutation");
    pMutation->SetAttribute("detected", r.getDetected());

    addChildToParent(doc, pMutation, "sourceFile",
        r.getMutant().getPath().filename());
    addChildToParent(doc, pMutation, "sourceFilePath",
                     os::path::getRelativePath(r.getMutant().getPath(),
                                               mSourcePath));
    addChildToParent(doc, pMutation, "mutatedClass", r.getMutant().getClass());
    addChildToParent(doc, pMutation, "mutatedMethod",
                     r.getMutant().getFunction());
    addChildToParent(doc, pMutation, "lineNumber",
        std::to_string(r.getMutant().getFirst().line));
    addChildToParent(doc, pMutation, "mutator", r.getMutant().getOperator());
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
