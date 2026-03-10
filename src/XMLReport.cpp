/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <tinyxml2/tinyxml2.h>
#include <memory>
#include <string>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/XMLReport.hpp"

namespace fs = std::filesystem;

namespace sentinel {

XMLReport::XMLReport(const MutationResults& results, const std::string& sourcePath) : Report(results, sourcePath) {}

XMLReport::XMLReport(const std::string& resultsPath, const std::string& sourcePath) : Report(resultsPath, sourcePath) {}

void XMLReport::save(const std::filesystem::path& dirPath) {
  mLogger->info("Make XML Report");
  if (fs::exists(dirPath)) {
    if (!fs::is_directory(dirPath)) {
      throw InvalidArgumentException(fmt::format("'{}' is not a directory", dirPath.string()));
    }
  } else {
    fs::create_directories(dirPath);
  }

  auto xmlPath = dirPath / "mutations.xml";

  auto doc = std::make_unique<tinyxml2::XMLDocument>();
  tinyxml2::XMLDeclaration* pDecl = doc->NewDeclaration();
  doc->InsertFirstChild(pDecl);

  tinyxml2::XMLElement* pMutations = doc->NewElement("mutations");

  for (const auto& r : mResults) {
    auto currentState = r.getMutationState();
    bool skip = false;
    if (currentState == MutationState::BUILD_FAILURE || currentState == MutationState::RUNTIME_ERROR ||
        currentState == MutationState::TIMEOUT) {
      skip = true;
    }

    tinyxml2::XMLElement* pMutation = doc->NewElement("mutation");
    if (skip) {
      pMutation->SetAttribute("detected", "skip");
    } else {
      pMutation->SetAttribute("detected", r.getDetected());
    }

    addChildToParent(doc.get(), pMutation, "sourceFile", r.getMutant().getPath().filename());
    addChildToParent(doc.get(), pMutation, "sourceFilePath", getRelativePath(r.getMutant().getPath(), mSourcePath));
    addChildToParent(doc.get(), pMutation, "mutatedClass", r.getMutant().getClass());
    addChildToParent(doc.get(), pMutation, "mutatedMethod", r.getMutant().getFunction());
    addChildToParent(doc.get(), pMutation, "lineNumber", std::to_string(r.getMutant().getFirst().line));
    addChildToParent(doc.get(), pMutation, "mutator", r.getMutant().getOperator());
    if (skip) {
      addChildToParent(doc.get(), pMutation, "killingTest", "");
    } else {
      addChildToParent(doc.get(), pMutation, "killingTest", r.getKillingTest());
    }

    pMutations->InsertEndChild(pMutation);
  }

  doc->InsertEndChild(pMutations);
  doc->SaveFile(xmlPath.c_str());
  mLogger->info("Save to {}", xmlPath.string());
}

void XMLReport::addChildToParent(tinyxml2::XMLDocument* d, tinyxml2::XMLElement* p, const std::string& childName,
                                 const std::string& childText) {
  tinyxml2::XMLElement* pChild = d->NewElement(childName.c_str());
  pChild->SetText(childText.c_str());
  p->InsertEndChild(pChild);
}

}  // namespace sentinel
