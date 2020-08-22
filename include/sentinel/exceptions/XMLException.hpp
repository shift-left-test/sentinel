/*
  MIT License

  Copyright (c) 2020 Sangmo Kang

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

#ifndef INCLUDE_SENTINEL_EXCEPTIONS_XMLEXCEPTION_HPP_
#define INCLUDE_SENTINEL_EXCEPTIONS_XMLEXCEPTION_HPP_

#include <tinyxml2/tinyxml2.h>
#include <stdexcept>
#include <string>


namespace sentinel {

/**
 * @brief XMLException class
 */
class XMLException : public std::runtime_error {
 public:
  /**
   * @brief Default constructor
   *
   * @param error code
   */
  explicit XMLException(tinyxml2::XMLError error) :
      std::runtime_error(tinyxml2::XMLDocument::ErrorIDToName(error)) {
  }

  /**
   * @brief Default constructor
   *
   * @param xmlPath
   * @param error code
   */
  explicit XMLException(const std::string& xmlPath, tinyxml2::XMLError error) :
      std::runtime_error(
      std::string().append(xmlPath).append(" : ").append(
      tinyxml2::XMLDocument::ErrorIDToName(error))
      ) {
  }

  /**
   * @brief Default constructor
   *
   * @param xmlPath
   * @param errorMessage
   */
  explicit XMLException(const std::string& xmlPath,
      const std::string& errorMessage) : std::runtime_error(
      std::string().append(xmlPath).append(" : ").append(errorMessage)) {
  }
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_EXCEPTIONS_XMLEXCEPTION_HPP_
