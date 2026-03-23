/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_XMLREPORT_HPP_
#define INCLUDE_SENTINEL_XMLREPORT_HPP_

#include <tinyxml2/tinyxml2.h>
#include <filesystem>  // NOLINT
#include <string>
#include "sentinel/Report.hpp"

namespace sentinel {

/**
 * @brief XmlReport class
 */
class XmlReport : public Report {
 public:
  /**
   * @brief Construct an XML report view from a precomputed summary.
   *
   * @param summary Aggregated mutation data to render.
   */
  explicit XmlReport(const MutationSummary& summary);

  /**
   * @brief Save the report in XML format to @p dirPath.
   *
   * @param dirPath Directory to save mutations.xml into.
   * @throw InvalidArgumentException when path is not a directory.
   */
  void save(const std::filesystem::path& dirPath) override;

 private:
  /**
   * @brief add Child Element To Parent Element
   *
   * @param d XMLDocument that has XMLElement p
   * @param p XMLElemnet that has a new child
   * @param childName child XMLElement's name
   * @param childText child XMLElement's text
   */
  void addChildToParent(tinyxml2::XMLDocument* d, tinyxml2::XMLElement* p, const std::string& childName,
                        const std::string& childText);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_XMLREPORT_HPP_
