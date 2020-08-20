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

#ifndef INCLUDE_SENTINEL_GITREPOSITORY_HPP_
#define INCLUDE_SENTINEL_GITREPOSITORY_HPP_

#include <memory>
#include <string>
#include "sentinel/Repository.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/Logger.hpp"


namespace sentinel {
/**
 * @brief RepositoryExcpetion class
 */
class RepositoryException : public ::std::exception {
  std::string msg;

 public:
  /**
   * @brief Default constructor
   *
   * @param msg excption description
   */
  explicit RepositoryException(const std::string & msg);

  ///@ private
  const char * what() const noexcept override {
    return this->msg.c_str();
  }
};

/**
 * @brief GitRepository class
 */
class GitRepository : public Repository {
 public:
  /**
   * @brief Default constructor
   *
   * @param path to the git repository
   */
  explicit GitRepository(const std::string& path);
  ~GitRepository();

  /**
   * @brief Return the source lines
   *
   * @return SourceLines object
   */
  SourceLines getSourceLines();
  std::shared_ptr<SourceTree> getSourceTree() override;

 private:
  std::string path_;

  std::shared_ptr<Logger> logger_;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_GITREPOSITORY_HPP_
