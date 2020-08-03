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

#ifndef INCLUDE_SENTINEL_UTIL_FILESYSTEM_HPP_
#define INCLUDE_SENTINEL_UTIL_FILESYSTEM_HPP_

#include <unistd.h>
#include <string>


namespace sentinel {
namespace util {
namespace filesystem {

/**
 * @brief The separator for the filesystem paths
 */
static constexpr const char SEPARATOR = '/';

/**
 * @brief The dot for the filesystem paths
 */
static constexpr const char DOT = '.';

/**
 * @brief Check if the given file exists
 *
 * @param path to file
 * @return true if the file exists, false otherwise
 */
inline bool exists(const std::string& path) {
  return access(path.c_str(), 0) == 0;
}

/**
 * @brief Check if the given file is a directory
 *
 * @param path to file
 * @return true if the file is a directory, false otherwise
 */
inline bool isDirectory(const std::string& path) {
  struct stat sb = {};
  lstat(path.c_str(), &sb);
  return S_ISDIR(sb.st_mode) != 0;
}

/**
 * @brief Check if the given file is a regular file
 *
 * @param path to file
 * @return true if the file is a regular file, false otherwise
 */
inline bool isRegularFile(const std::string& path) {
  struct stat sb = {};
  lstat(path.c_str(), &sb);
  return S_ISREG(sb.st_mode) != 0;
}

/**
 * @brief Return the directory name of the given path
 *
 * @param path to file
 * @return directory name
 */
inline std::string dirname(const std::string& path) {
  auto position = path.find_last_of(SEPARATOR, path.size() - 2);
  if (position == std::string::npos) {
    return ".";
  }
  if (position == 0) {
    return "/";
  }
  return path.substr(0, position);
}

/**
 * @brief Return the filename of the given path
 *
 * @param path to file
 * @return filename
 */
inline std::string filename(std::string path) {
  while (path.back() == SEPARATOR) {
    path.pop_back();
  }
  auto position = path.find_last_of(SEPARATOR);
  return (position == std::string::npos) ? path : path.substr(position + 1);
}

}  // namespace filesystem
}  // namespace util
}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_UTIL_FILESYSTEM_HPP_
