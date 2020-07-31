#ifndef GIT_HARNESS_H
#define GIT_HARNESS_H

#include <string>
#include <vector>
#include <git2.h>

bool directoryExists(const std::string &directory);
bool fileExists(const std::string &name);
void executeSystemCommand(std::string command, std::string error_msg = "");
void libgitErrorCheck(int error, const char *msg);
void convertToCharArray(std::vector<std::string> *from, std::vector<char*> *to);

class GitHarness
{
public:
  GitHarness():repo(nullptr) { git_libgit2_init(); }
  ~GitHarness() { git_libgit2_shutdown(); }
  
  // Create a folder and git init.
  // By default, git repo /temp/temp_repo is created.
  GitHarness& initiateGitRepo(std::string dir_name = "/temp/temp_repo");
  
  // Create a folder within git directory.
  // If folder name does not start with /, repo_path is prepended to folder_name
  GitHarness& addFolder(std::string folder_name);

  // Create a file at specified location (w.r.t repo path)
  // Ex: call addFile("src/a.cpp") to add a file /temp/tempRepo/src/a.cpp
  // inside the git repo
  GitHarness& addFile(std::string filename, std::string content="");

  // Add code to file.
  // If line number is negative or exceed file length, the code is appended to 
  // end of file. By default, code is appended to end of file.
  // If column number is negative, code is appended to target line.
  // By default, column number is 0 and code is added to start of target line.
  GitHarness& addCode(std::string filename, std::string &content,
                      int line=-1, int col=0);

  // Delete code between (start_line, start_col) and (end_line, end_col).
  // A negative line number indicates end of file.
  // A negative column number indicates end of line.
  // GitHarness& deleteCode(std::string &filename, int start_line, 
  //                        int end_line, int start_col=-1, int end_col=-1);

  // Delete multiple line of codes. Code line starts from 1.
  GitHarness& deleteCode(std::string filename, std::vector<int> lines);  

  // git add <filenames>
  GitHarness& stageFile(std::vector<std::string> filenames);

  // git commit -m "messasge"
  GitHarness& commit(std::string message);
  
  // git tag -a <tag_name> -m "message" <commit_id>
  // GitHarness& addTag(std::string tag_name, std::string commit_id, 
  //                    std::string message="empty");

  // git tag <tag_name> <commit_id>
  GitHarness& addTagLightweight(std::string tag_name, git_oid commit_id);

  git_oid getLatestCommitId();
  git_repository* getGitRepo();

private:
  std::string repo_path;
  git_repository *repo;
  std::vector<git_oid> commit_ids;
};

enum index_mode {
  INDEX_NONE,
  INDEX_ADD,
};

struct index_options {
  int dry_run;
  int verbose;
  git_repository *repo;
  enum index_mode mode;
  int add_update;
};

#endif  // GIT_HARNESS_H
