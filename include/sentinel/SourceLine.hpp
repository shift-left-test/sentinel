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

#ifndef INCLUDE_SENTINEL_SOURCELINE_HPP_
#define INCLUDE_SENTINEL_SOURCELINE_HPP_

#include <experimental/filesystem>
#include <string>


namespace sentinel {

/**
 * @brief SourceLine class
 */
class SourceLine {
 public:
  /**
   * @brief Default constructor
   *
   * @param path source file path
   *
   * @param lineNumber soure line number
   */
  SourceLine(const std::experimental::filesystem::path& path,
      std::size_t lineNumber);

  /**
   * @brief == operator overloading for std::find algorithm
   *
   * @param other other SourceLine instance
   */
  bool operator ==(const SourceLine& other) const {
    return std::experimental::filesystem::equivalent(this->mPath, other.mPath)
      && this->mLineNumber == other.mLineNumber;
  }

  /**
   * @brief < operator overloading for std::find algorithm
   *
   * @param other other SourceLine instance
   */
  bool operator <(const SourceLine& other) const {
    if (this->mPath.string() < other.mPath.string()) {
      return true;
    }
    if (this->mPath.string() > other.mPath.string()) {
      return false;
    }
    return this->mLineNumber < other.mLineNumber;
  }

  /**
   * @brief Return path to file
   */
  std::experimental::filesystem::path getPath() const {
    return mPath;
  }

  /**
   * @brief Return line number
   */
  std::size_t getLineNumber() const {
    return mLineNumber;
  }

 private:
  std::experimental::filesystem::path mPath;
  std::size_t mLineNumber;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_SOURCELINE_HPP_
