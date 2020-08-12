#ifndef GIT_HARNESS_H
#define GIT_HARNESS_H

#include <string>
#include <vector>
#include <git2.h>

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
                      int line=-1, int col=0);

  /**
   * @brief Delete multiple line of codes within target file.
   *
   * @param filename relative file path (target file name included) with
   *                 with to git repo path.
   * @param lines    list of target lines to delete.
   */
  GitHarness& deleteCode(const std::string& filename,
		         const std::vector<int>& lines);

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

#endif  // GIT_HARNESS_H