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

#ifndef TEST_HARNESS_GIT_HARNESS_GITHARNESS_HPP_
#define TEST_HARNESS_GIT_HARNESS_GITHARNESS_HPP_

#include <git2.h>
#include <string>
#include <vector>


namespace sentinel {

class GitHarness {
 public:
  /**
   * @brief Default constructor. Create a new git repo with given folder name.
   *
   * @param dir_name name of new git folder.
   */
  explicit GitHarness(const std::string& dir_name);

  /**
   * @brief Default destructor
   */
  virtual ~GitHarness();

  /**
   * @brief Create a new folder within the git repo.
   *
   * @param folder_name relative path (new folder name included) with respect
   *                    to git repo path.
   */
  GitHarness& addFolder(const std::string& folder_name);

  /**
   * @brief Create a new file within the git repo and write content to file.
   *
   * @param filename relative file path (new file name included) with respect
   *                 to git repo path.
   * @param content  text to insert to new file.
   */
  GitHarness& addFile(const std::string& filename, const std::string& content);

  /**
   * @brief Insert code to file at specified location (if any).
   *        By default, code is appended to end of file.
   *        If line number of negative or exceed file length, code is appended
   *        to end of file.
   *        If column number of negative or exceeding line length, code is
   *        appended to end of target line.
   *
   * @param filename relative file path (file name included) with respect
   *                 to git repo pat.
   * @param content  text to insert to target file.
   * @param line     line number of target location to insert code.
   * @param col      column number of target location to insert code.
   */
  GitHarness& addCode(const std::string& filename, const std::string& content,
                      std::size_t line = 0, std::size_t col = 0);

  /**
   * @brief Delete multiple line of codes within target file.
   *
   * @param filename relative file path (target file name included) with
   *                 with to git repo path.
   * @param lines    list of target lines to delete.
   */
  GitHarness& deleteCode(const std::string& filename,
                         const std::vector<std::size_t>& lines);

  /**
   * @brief Implementation of git add <filenames>.
   *
   * @param filenames list of relative file paths (target file name included)
   *                  with respect to git repo path.
   */
  GitHarness& stageFile(std::vector<std::string> filenames);

  /**
   * @brief Implementation of git commit -m <message>
   *
   * @param message commit message.
   */
  GitHarness& commit(const std::string& message);

  /**
   * @brief Implementation of git tag <tag name> <commit id>.
   *
   * @param tag_name tag name.
   * @param oid_str  target commit id.
   */
  GitHarness& addTagLightweight(const std::string& tag_name,
                                const std::string& oid_str);

  /**
   * @brief Implementation of git tag <tag name>. Tag the head commit.
   *
   * @param tag_name tag name.
   */
  GitHarness& addTagLightweight(const std::string& tag_name);

  /**
   * @brief Implementation of git checkout -b <branch_name>.
   *
   * @param branch_name name of new branch
   * @param commit_id   ID of commit from which new branch is created.
   */
  GitHarness& createBranch(const std::string& branch_name);
  GitHarness& createBranch(const std::string& branch_name,
                           const std::string& commit_id);

  /**
   * @brief Implementation of git checkout <branch_name>.
   *        Checkout an existing branch.
   *
   * @param branch_name name of target branch
   */
  GitHarness& checkoutBranch(const std::string& branch_name);

  /**
   * @brief Implementation of git merge <branch1> [<branch2> ...]
   *
   * @param branches names of branches to merge to HEAD
   */
  GitHarness& merge(const std::string& branch);
  GitHarness& merge(const std::vector<std::string>& branches);

  template <typename ... Arg>
  inline GitHarness& merge(const Arg&... branches) {
    std::vector<std::string> v = {branches ...};  // NOLINT
    return merge(v);
  }


  /**
   * @brief Get HEAD commit id.
   */
  std::string getLatestCommitId();

  /**
   * @brief Get pointer to Git Repository object.
   */
  git_repository* getGitRepo();

  /**
   * @brief Check the error code returned by a libgit command.
   *        Print error message if the error code indicates failure.
   *
   * @param error error code.
   * @param msg   error message to print if error happens.
   */
  static void libgitErrorCheck(int error, const char* msg);

 private:
  std::string repo_path;
  git_repository* repo;
  std::vector<git_oid> commit_ids;
};


enum index_mode {
  INDEX_NONE,
  INDEX_ADD,
};

struct index_options {
  int dry_run;
  int verbose;
  git_repository* repo;
  enum index_mode mode;
  int add_update;
};

}  // namespace sentinel

#endif  // TEST_HARNESS_GIT_HARNESS_GITHARNESS_HPP_
