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

/**
 * @brief Location struct
 */
struct Location {
  /**
   * @brief Default constructor
   *
   * @param lineNum number
   * @param colNum number
   */
  Location(int lineNum, int colNum) : line(lineNum), column(colNum) {}

  /**
   * @brief line number
   */
  int line;

  /**
   * @brief column number
   */
  int column;
};

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
   * @param first location to apply mutation
   * @param last location to apply mutation
   * @param token to replace with
   */
  Mutable(const std::string& path,
          const Location& first,
          const Location& last,
          const std::string& token);

  /**
   * @brief Default constructor
   *
   * @param path to file from the source tree
   * @param firstLine line number of first location
   * @param firstColumn column number of first location
   * @param lastLine line number of last location
   * @param lastColumn column number of last location
   * @param token to replace with
   */
  Mutable(const std::string& path,
          int firstLine,
	  int firstColumn,
          int lastLine,
          int lastColumn,
          const std::string& token);

  /**
   * @brief Return the path of mutated file.
   *
   * @return path to the file
   */
  std::string getPath() const;

  /**
   * @brief Return the first location to apply mutation.
   *
   * @return line and column number of first location.
   */
  Location getFirst() const;

  /**
   * @brief Return the last location to apply mutation.
   *
   * @return line and column number of last location.
   */
  Location getLast() const;

  /**
   * @brief Return the token
   *
   * @return the token
   */
  std::string getToken() const;

 private:
  std::string mPath;
  Location mFirst;
  Location mLast;
  std::string mToken;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTABLE_HPP_
