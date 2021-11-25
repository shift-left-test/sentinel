/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_XMLREPORT_HPP_
#define INCLUDE_SENTINEL_XMLREPORT_HPP_

#include <tinyxml2/tinyxml2.h>
#include <string>
#include "sentinel/Report.hpp"


namespace sentinel {

/**
 * @brief XMLReport class
 */
class XMLReport : public Report {
 public:
  /**
   * @brief Default constructor
   *
   * @param results mutation results
   * @param sourcePath directory path of source files
   */
  XMLReport(const MutationResults& results, const std::string& sourcePath);

  /**
   * @brief Default constructor
   *
   * @param resultsPath directory path of mutation results
   * @param sourcePath directory path of source files
   */
  XMLReport(const std::string& resultsPath, const std::string& sourcePath);

  /**
   * @brief save xml format result to path
   *
   * @param dirPath
   *
   * @throw InvalidArgumentException when path is not directory
   */
  void save(const std::experimental::filesystem::path& dirPath) override;

 private:
  /**
   * @brief add Child Element To Parent Element
   *
   * @param d XMLDocument that has XMLElement p
   * @param p XMLElemnet that has a new child
   * @param childName child XMLElement's name
   * @param childText child XMLElement's text
   */
  void addChildToParent(tinyxml2::XMLDocument* d, tinyxml2::XMLElement* p,
      const std::string& childName, const std::string& childText);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_XMLREPORT_HPP_
