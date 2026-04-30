/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <tinyxml2/tinyxml2.h>
#include <fmt/core.h>
#include <cerrno>
#include <cstring>
#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include "sentinel/MutationResult.hpp"
#include "sentinel/XmlReport.hpp"
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/util/io.hpp"

namespace sentinel {

namespace fs = std::filesystem;

XmlReport::XmlReport(const MutationSummary& summary) : Report(summary) {
}

void XmlReport::save(const std::filesystem::path& dirPath) {
  io::ensureDirectoryExists(dirPath);

  auto xmlPath = dirPath / "mutations.xml";

  auto doc = std::make_unique<tinyxml2::XMLDocument>();
  tinyxml2::XMLDeclaration* pDecl = doc->NewDeclaration();
  doc->InsertFirstChild(pDecl);

  tinyxml2::XMLElement* pMutations = doc->NewElement("mutations");

  for (const auto& r : mSummary.results) {
    auto state = r.getMutationState();
    bool skip = state == MutationState::BUILD_FAILURE ||
                state == MutationState::RUNTIME_ERROR ||
                state == MutationState::TIMEOUT;

    tinyxml2::XMLElement* pMutation = doc->NewElement("mutation");
    if (skip) {
      pMutation->SetAttribute("detected", "skip");
    } else {
      pMutation->SetAttribute("detected", r.getDetected());
    }

    if (state == MutationState::SURVIVED && r.isUncovered()) {
      pMutation->SetAttribute("uncovered", "true");
    }

    addChildToParent(doc.get(), pMutation, "sourceFile", r.getMutant().getPath().filename().string());
    addChildToParent(doc.get(), pMutation, "sourceFilePath", r.getMutant().getPath().string());
    addChildToParent(doc.get(), pMutation, "mutatedClass", r.getMutant().getClass());
    addChildToParent(doc.get(), pMutation, "mutatedMethod", r.getMutant().getFunction());
    addChildToParent(doc.get(), pMutation, "lineNumber", std::to_string(r.getMutant().getFirst().line));
    addChildToParent(doc.get(), pMutation, "mutator", r.getMutant().getOperator());
    addChildToParent(doc.get(), pMutation, "killingTest", skip ? "" : r.getKillingTest());

    pMutations->InsertEndChild(pMutation);
  }

  doc->InsertEndChild(pMutations);
  if (doc->SaveFile(xmlPath.c_str()) != tinyxml2::XML_SUCCESS) {
    throw IOException(errno, fmt::format("Failed to write '{}': {}",
                                         xmlPath.string(), std::strerror(errno)));
  }
}

void XmlReport::addChildToParent(tinyxml2::XMLDocument* d, tinyxml2::XMLElement* p, const std::string& childName,
                                 const std::string& childText) {
  tinyxml2::XMLElement* pChild = d->NewElement(childName.c_str());
  pChild->SetText(childText.c_str());
  p->InsertEndChild(pChild);
}

}  // namespace sentinel
