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

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <string>
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/util/string.hpp"

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
    return std::string(1, DOT);
  }
  if (position == 0) {
    return std::string(1, SEPARATOR);
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

/**
 * @brief Join the given paths with slash ('/')
 *
 * @param path1 the first path
 * @param paths the other paths, in order.
 * @return joined path
 */
template <typename ... Arg>
inline std::string join(const std::string& path1,
                        const Arg&... paths) {
  return string::join(SEPARATOR, {path1, paths ...});  // NOLINT
}

/**
 * @brief Create a new directory at given path.
 *
 * @param path to new folder (name included).
 * @throw IOException given path already exists, or
 *                    a component of the path prefix does not exist, or
 *                    a component of the path prefix is not a director.
 */
inline void createDirectory(const std::string& path) {
  if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0) {
    throw IOException(errno);
  }
}

/**
 * @brief Create a new directory at given path.
 *        Create parent directory if needed.
 *
 * @param path to new folder.
 * @throw IOException given path already exists.
 */
inline void createDirectories(const std::string& path) {
  bool existed = true;
  for (auto iter = path.begin(); iter != path.end(); ) {
    auto nextSlash = std::find(iter, path.end(), SEPARATOR);
    std::string currentPath = std::string(path.begin(), nextSlash);

    if (!exists(currentPath)) {
      existed = false;
      createDirectory(currentPath);
    }

    iter = nextSlash;
    if (iter != path.end()) {
      ++iter;
    }
  }

  if (existed) {
    throw IOException(EEXIST);
  }
}

/**
 * @brief Delete a specified empty directory.
 *
 * @param path to target directory.
 * @throw IOException given path is not an empty directory, or
 *                    given path last component is . or .., or
 *                    given path is not a directory, or
 *                    given path does not exist.
 */
inline void removeDirectory(const std::string& path) {
  if (rmdir(path.c_str()) != 0) {
    throw IOException(errno);
  }
}

/**
 * @brief Delete a specified file.
 *
 * @param path to target directory.
 * @throw IOException given path is not a file, or
 *                    given path does not exist.
 */
inline void removeFile(const std::string& path) {
  if (std::remove(path.c_str()) != 0) {
    throw IOException(errno);
  }
}

/**
 * @brief Delete a directory at given path.
 *        Recursively delete the directory content before deleting it.
 *
 * @param path to new folder.
 */
inline void removeDirectories(const std::string& path) {
  DIR* d = opendir(path.c_str());
  if (d != nullptr) {
    struct dirent* entry = nullptr;
    while ((entry = readdir(d)) != nullptr) {
      if (std::strcmp(static_cast<const char *>(entry->d_name), ".") == 0 ||
          std::strcmp(static_cast<const char *>(entry->d_name), "..") == 0) {
        continue;
      }
      auto subpath = join(path, entry->d_name);
      if (isDirectory(subpath)) {
        removeDirectories(subpath);
      } else {
        removeFile(subpath);
      }
    }
    closedir(d);
  }
  removeDirectory(path);
}

/**
 * @brief Create a temporary file name with given template.
 *
 * @param prefix front part of file name template, before XXXXXX
 * @return temporary file name.
 * @throw IOException a unique name cannot be created.
 */
inline std::string tempPath(const std::string& prefix = "") {
  auto path = prefix + "XXXXXX";
  if (mkstemp(&path[0]) == -1) {
    throw IOException(errno);
  }
  removeFile(path);
  return path;
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
 * @brief Replace the file dest with the file src.
 *
 * @param src replacing file
 * @param dest to-be-replaced file
 * @throw IOException src is not an exising file, or
 *                    dest is an existing directory.
 */
inline void rename(const std::string& src, const std::string& dest) {
  if (!isRegularFile(src)) {
    throw IOException(EINVAL);
  }
  if (std::rename(src.c_str(), dest.c_str()) != 0) {
    throw IOException(errno);
  }
}

}  // namespace filesystem
}  // namespace util
}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_UTIL_FILESYSTEM_HPP_
