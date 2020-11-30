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
