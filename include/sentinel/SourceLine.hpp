/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
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
  SourceLine(const std::experimental::filesystem::path& path, std::size_t lineNumber);

  /**
   * @brief == operator overloading for std::find algorithm
   *
   * @param other other SourceLine instance
   */
  bool operator ==(const SourceLine& other) const {
    return std::experimental::filesystem::equivalent(this->mPath, other.mPath) &&
        this->mLineNumber == other.mLineNumber;
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
