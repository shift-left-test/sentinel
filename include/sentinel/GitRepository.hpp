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
#include "sentinel/exceptions/RepositoryException.hpp"
#include "sentinel/Repository.hpp"
#include "sentinel/SourceLines.hpp"
#include "sentinel/SourceTree.hpp"
#include "sentinel/Logger.hpp"


namespace sentinel {

/**
 * @brief GitRepository class
 */
class GitRepository : public Repository {
 public:
  /**
   * @brief Default constructor
   *
   * @param path to the directory repository
   * @param extensions compile_commands.json file location
   * @param excludes excluded directory list
   */
  explicit GitRepository(const std::string& path,
    const std::vector<std::string>& extensions=std::vector<std::string>(),
    const std::vector<std::string>& excludes=std::vector<std::string>());

  /**
   * @brief Default destructor
   */
  virtual ~GitRepository();

  std::shared_ptr<SourceTree> getSourceTree() override;
  
  /**
   * @brief Return the diff source lines from commit tree.
   * 
   * @return SourceLines object
   */
  SourceLines getSourceLines(const std::string& scope) override;

  /**
   * @brief Return absolute root path
   */
  const std::string& getSourceRoot() { return mSourceRoot; }

  /**
   * @brief Return path is target path for getSourceLines
   *
   * @param path 
   * 
   * @param checkExtension if true, 
   *        check extensino of path is included by extensions_.
   * 
   * @return return true if path is valid sourceline target.
   */
  bool isTargetPath(const std::string &path,
    bool checkExtension=true);

 private:
  std::string mSourceRoot;
  std::vector<std::string> mExtensions;
  std::vector<std::string> mExcludes;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_GITREPOSITORY_HPP_
