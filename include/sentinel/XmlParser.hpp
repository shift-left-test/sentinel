/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_XMLPARSER_HPP_
#define INCLUDE_SENTINEL_XMLPARSER_HPP_

#include <tinyxml2/tinyxml2.h>
#include <memory>
#include <string>
#include <vector>

namespace sentinel {

/**
 * @brief Abstract XML parser class
 */
class XmlParser {
 public:
  /**
   * @brief Default constructor
   */
  XmlParser();

  /**
   * @brief Default destructor
   */
  virtual ~XmlParser() = default;

  /**
   * @brief Set the next XML parser
   *
   * @param parser next parser
   * @return pointer to the next parser
   */
  std::shared_ptr<XmlParser> setNext(std::shared_ptr<XmlParser> parser);

  /**
   * @brief Process the given XML file to collect passed and failed test result data
   *
   * @param path to the XML file
   * @param passed tests
   * @param failed tests
   */
  void process(const std::string& path, std::vector<std::string>* passed, std::vector<std::string>* failed);

 protected:
  /**
   * @brief Parse the given XML data to collect test results
   *
   * @param document of an XML file to parse
   * @return true if parsing is successful, false otherwise
   */
  virtual bool parse(std::shared_ptr<tinyxml2::XMLDocument> document) = 0;

  /**
   * @brief Add the name of passed tests
   *
   * @param name of the test
   */
  void addPassed(const std::string& name);

  /**
   * @brief Add the name of failed tests
   *
   * @param name of the test
   */
  void addFailed(const std::string& name);

 private:
  /**
   * @brief Reset the previously processed data
   */
  void reset();

  /**
   * @brief Collect the test result data
   *
   * @param passed[in] tests
   * @param failed[in] tests
   */
  void collect(std::vector<std::string>* passed, std::vector<std::string>* failed);

  std::shared_ptr<XmlParser> mNext;
  std::vector<std::string> mPassed;
  std::vector<std::string> mFailed;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_XMLPARSER_HPP_
