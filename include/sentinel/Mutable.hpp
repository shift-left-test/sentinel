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

#ifndef INCLUDE_SENTINEL_MUTABLE_HPP_
#define INCLUDE_SENTINEL_MUTABLE_HPP_

#include <string>
#include <utility>
#include "sentinel/SourceTree.hpp"


namespace sentinel {

class SourceTree;

/**
 * @brief Mutable class
 */
class Mutable {
 public:
  /**
   * @brief Default constructor
   *
   * @param path to file from the source tree
   * @param lineNumber line number for mutation
   * @param column column index (from, to)
   * @param token to replace with
   */
  Mutable(const std::string& path,
          int lineNumber,
          const std::pair<int, int>& column,
          const std::string& token);

  /**
   * @brief Default constructor
   *
   * @param path to file from the source tree
   * @param lineNumber line number for mutation
   * @param first column index
   * @param last column index
   * @param token to replace with
   */
  Mutable(const std::string& path,
          int lineNumber,
          int first,
          int last,
          const std::string& token);

  /**
   * @brief Return the path
   *
   * @return path to the file
   */
  std::string getPath() const;

  /**
   * @brief Return the line number
   *
   * @return the line number
   */
  int getLineNumber() const;

  /**
   * @brief Return the column information
   *
   * @return the column information (from, to)
   */
  std::pair<int, int> getColumn() const;

  /**
   * @brief Return the token
   *
   * @return the token
   */
  std::string getToken() const;

  /**
   * @brief Mutate the given source tree based on the mutable information
   *
   * @param sourceTree to mutate
   */
  void mutate(SourceTree sourceTree);

 private:
  std::string mPath;
  int mLineNumber;
  std::pair<int, int> mColumn;
  std::string mToken;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTABLE_HPP_
