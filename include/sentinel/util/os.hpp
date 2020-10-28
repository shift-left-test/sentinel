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

#ifndef INCLUDE_SENTINEL_UTIL_OS_HPP_
#define INCLUDE_SENTINEL_UTIL_OS_HPP_

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <experimental/filesystem>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <regex>
#include <string>
#include <vector>
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/util/string.hpp"


namespace fs = std::experimental::filesystem;

namespace sentinel {
namespace os {
namespace path {

/**
 * @brief The separator for the filesystem paths
 */
static constexpr const char SEPARATOR = '/';

/**
 * @brief The dot for the filesystem paths
 */
static const char* DOT = ".";

/**
 * @brief The doulbe dots for the filesystem paths
 */
static const char* DOUBLEDOTS = "..";

/**
 * @brief Return the relative path from start dir
 *
 * @param path target path
 * @param path start dir
 * @return the relative path from start dir
 */
inline fs::path getRelativePath(const std::string& path,
                                   const std::string& start) {
  auto mPath = fs::canonical(path);
  auto mBase = fs::canonical(start);
  
  fs::path resultPath;
  auto itPath = mPath.begin();
  auto itBase = mBase.begin();

  for (; itPath != mPath.end() && itBase != mBase.end(); ++itPath, ++itBase) {
    if (*itPath != *itBase) {
      break;
    }
  }

  for (; itBase != mBase.end(); ++itBase) {
    if (*itBase != ".") {
      resultPath /= "..";
    }
  }

  for(; itPath != mPath.end(); ++itPath) {
    if (*itPath != ".") {
      resultPath /= *itPath;
    }
  }

  return resultPath.empty() ? "." : resultPath;
}

}  // namespace path

/**
 * @brief Create a temporary file name with given template.
 *
 * @param prefix front part of file name template
 * @param suffix end part of file name template
 * @return temporary file name.
 * @throw IOException a unique name cannot be created.
 */
inline std::string tempPath(const std::string& prefix = "",
                            const std::string& suffix = "") {
  std::string s =
      "abcdefghijklmnoperstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  std::string temp;

  std::random_device r;
  std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
  std::mt19937 eng(seed);

  do {
    std::shuffle(s.begin(), s.end(), eng);
    temp = prefix + s.substr(0, 8) + suffix;
  } while (fs::exists(temp));
  return temp;
}

/**
 * @brief Create a temporary file with the given template and given suffix.
 *
 * @param prefix of the filename
 * @param suffix of the filename
 * @return temporary filename
 * @throw IOException if not able to create a file
 */
inline std::string tempFilename(const std::string& prefix = "",
    const std::string& suffix = "") {

  auto retPath = tempPath(prefix, suffix);
  std::fstream fs(retPath, std::fstream::out);
  fs.close();
  return retPath;
}

/**
 * @brief Create a temporary directory whose name follows the given template.
 *
 * @param prefix template of directory name.
 * @return temporary directory name.
 * @throw IOException a unique name cannot be created.
 */
inline std::string tempDirectory(const std::string& prefix = "") {
  auto path = prefix + "XXXXXX";
  if (mkdtemp(&path[0]) == nullptr) {
    throw IOException(errno);
  }
  return path;
}

/**
 * @brief find files that match regex
 *
 * @param dir search target directory
 * @return path of files that match regex
 */
inline std::vector<std::string> findFilesInDirUsingRgx(
    const fs::path& dir, const std::regex& exp) {
  std::vector<std::string> files;

  for(auto& dirent: fs::recursive_directory_iterator(dir)) {
    auto entry = dirent.path();
    if (fs::is_regular_file(entry)) {
      if (std::regex_match(entry.filename().string(), exp)) {
          files.push_back(entry);
      }
    }
  }
  
  return files;
}

/**
 * @brief Get files that have specific extension
 *
 * @param dir search target directory
 * @return path of files that have specific extension
 */
inline std::vector<std::string> findFilesInDirUsingExt(
    const fs::path& dir, const std::vector<std::string>& exts) {
  std::vector<std::string> files;

  if (exts.empty()) {
    for(auto& dirent: fs::recursive_directory_iterator(dir)) {
      auto entry = dirent.path();
      if (fs::is_regular_file(entry)) {
        files.push_back(entry);
      }
    }
  } else {
    std::vector<std::string> tmpExts;
    for(auto& ext: exts) {
      std::string tmp(ext);
      std::transform(tmp.begin(), tmp.end(), tmp.begin(),
        [](unsigned char c) { return std::tolower(c); });
      tmpExts.push_back(tmp);
    }

    for(auto& dirent: fs::recursive_directory_iterator(dir)) {
      auto entry = dirent.path();
      if (fs::is_regular_file(entry)) {
        std::string ext = entry.extension();
        if (!ext.empty()) {
          ext = ext.substr(1);
          std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return std::tolower(c); });
          if (std::find(tmpExts.begin(), tmpExts.end(), ext)
            != tmpExts.end()) {
            files.push_back(entry);
          }
        }
      }
    }
  }
  
  return files;
}

/**
 * @brief Get files in directory
 *
 * @param dir search target directory
 * @return path of files
 */
inline std::vector<std::string> findFilesInDir(
    const std::string& dir) {
  return findFilesInDirUsingExt(dir, {});
}

}  // namespace os
}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_UTIL_OS_HPP_
