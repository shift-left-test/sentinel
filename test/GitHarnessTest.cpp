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

#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>

#include "harness/git_harness.hpp"

class GitHarnessTest : public ::testing::Test {
 protected:
  GitHarnessTest() : repo_name("temp_repo") {}
  ~GitHarnessTest() {}

  void SetUp() override {
    executeSystemCommand("rm -rf temp_repo");
    repo = new GitHarness;  //NOLINT
  }

  void TearDown() override {
    executeSystemCommand("rm -rf temp_repo");
  }

  static void getStagedAndUnstagedFiles(
      std::vector<std::string> *staged_files,
      std::vector<std::string> *unstaged_files,
      git_status_list *status) {
    size_t i, maxi = git_status_list_entrycount(status);
    const git_status_entry *s;
    const char *old_path, *new_path;

    for (i = 0; i < maxi; ++i) {
      s = git_status_byindex(status, i);

      // A staged file is a file that is added, modified , delted, renamed, or
      // typechanged in git repo index.
      if (s->status != GIT_STATUS_CURRENT &&
          // (s->status & GIT_STATUS_WT_DELETED == 0) &&
          (s->status & GIT_STATUS_INDEX_NEW != 0 ||
           s->status & GIT_STATUS_INDEX_MODIFIED != 0 ||
           s->status & GIT_STATUS_INDEX_DELETED != 0 ||
           s->status & GIT_STATUS_INDEX_RENAMED != 0 ||
           s->status & GIT_STATUS_INDEX_TYPECHANGE != 0)) {
        old_path = s->head_to_index->old_file.path;
        new_path = s->head_to_index->new_file.path;
        staged_files->push_back((old_path ? old_path : new_path));
      }

      // An unstaged file is a file that is modified, deleted, renamed or
      // typechanged in git repo worktree
      if (s->status != GIT_STATUS_CURRENT && s->index_to_workdir != NULL &&
          (s->status & GIT_STATUS_WT_MODIFIED != 0 ||
           s->status & GIT_STATUS_WT_DELETED != 0 ||
           s->status & GIT_STATUS_WT_RENAMED != 0 ||
           s->status & GIT_STATUS_WT_TYPECHANGE != 0)) {
        old_path = s->index_to_workdir->old_file.path;
        new_path = s->index_to_workdir->new_file.path;
        unstaged_files->push_back((old_path ? old_path : new_path));
      }

      // An untracked file is a new file in the work tree
      if (s->status == GIT_STATUS_WT_NEW) {
        old_path = s->index_to_workdir->old_file.path;
        unstaged_files->push_back(old_path);
      }
    }
  }

  bool string_comparator(const std::string &left, const std::string &right) {
    return left.compare(right) == 0;
  }

  std::string repo_name;
  GitHarness *repo;
};

// Action: create a new directory and git init
// Expected Output: new directory created, containing .git folder
TEST_F(GitHarnessTest, init_Normal) {
  repo->initiateGitRepo(repo_name);
  EXPECT_TRUE(directoryExists(repo_name));

  std::string git_dir = repo_name + "/.git";
  EXPECT_TRUE(directoryExists(git_dir));
}

// Action: initiate a git repo twice
// Expected Output: assertion error
TEST_F(GitHarnessTest, init_DirAlreadyExist) {
  repo->initiateGitRepo(repo_name);
  EXPECT_DEATH(repo->initiateGitRepo(repo_name), "Assertion.*!directoryExists");
}

// Action: execute any GitHarness function before calling initiateGitRepo()
// Expected Output: assertion errors
TEST_F(GitHarnessTest, init_NotCalled) {
  EXPECT_DEATH(repo->addFolder("temp"), "repo != nullptr");
  EXPECT_DEATH(repo->addFile("temp.cpp"), "repo != nullptr");

  std::string content{"int main() {}"};
  std::vector<int> target_lines;
  std::vector<std::string> target_files{"temp.cpp"};
  EXPECT_DEATH(repo->addCode("temp.cpp", content), "repo != nullptr");
  EXPECT_DEATH(repo->deleteCode("temp.cpp", target_lines), "repo != nullptr");
  EXPECT_DEATH(repo->stageFile(target_files), "repo != nullptr");
  EXPECT_DEATH(repo->commit("this should fail"), "repo != nullptr");

  git_oid oid;
  EXPECT_DEATH(repo->addTagLightweight("fail_test", oid), "repo != nullptr");
}

// Action: create new folder in git repo
// Expected Output: folder created at within repo at correct location
TEST_F(GitHarnessTest, addFolder_Normal) {
  repo->initiateGitRepo(repo_name);
  repo->addFolder("temp");
  EXPECT_TRUE(directoryExists("temp_repo/temp"));
}

// Action: call addFolder with a folder that already exist
// Expected Output: assertion errors
TEST_F(GitHarnessTest, addFolder_FolderExisted) {
  repo->initiateGitRepo(repo_name);
  repo->addFolder("temp");
  EXPECT_DEATH({repo->addFolder("temp");}, "Assertion.*!directoryExists");
}

// Action: call addFolder with invalid path
// Expected Output: assertion errors
TEST_F(GitHarnessTest, addFolder_InvalidPath) {
  repo->initiateGitRepo(repo_name);
  EXPECT_DEATH({repo->addFolder("unexisted/temp");}, "Fail to make directory");
}

// Action: create new file in git repo with empty main function
// Expected Output: cile created at within repo at correct location and content.
TEST_F(GitHarnessTest, addFile_Normal) {
  repo->initiateGitRepo(repo_name);
  std::string filename = "temp.cpp";
  std::string content = "int main() {}";
  repo->addFile(filename, content);

  filename = repo_name + "/" + filename;
  EXPECT_TRUE(fileExists(filename));

  std::ifstream created_file(filename.c_str());
  std::string inserted_content((std::istreambuf_iterator<char>(created_file)),
                               (std::istreambuf_iterator<char>()));
  created_file.close();
  EXPECT_EQ(content, inserted_content);
}

// Action: call addFile with a file that already exist
// Expected Output: assertion errors
TEST_F(GitHarnessTest, addFile_FileExisted) {
  repo->initiateGitRepo(repo_name);
  repo->addFile("temp.cpp");
  EXPECT_DEATH({repo->addFile("temp.cpp");}, "Assertion.*!fileExists");
}

// Action: call addFile with invalid path
// Expected Output: assertion errors
TEST_F(GitHarnessTest, addFile_InvalidPath) {
  repo->initiateGitRepo(repo_name);
  EXPECT_DEATH({repo->addFile("unexisted/temp.cpp");}, "Fail to make file");
}

// Action: add content to file
// Expected Output: content are added properly to the right file. Other files
// are unaffected.
TEST_F(GitHarnessTest, addCode_Normal) {
  repo->initiateGitRepo(repo_name);
  std::string filename = repo_name + "/" + "temp.cpp";
  std::string initial_content = "// comment\n";
  repo->addFile("temp.cpp", initial_content);

  // Default behavior: add code to end of file if no position is specified
  std::string content = "int main() {}\n";
  repo->addCode("temp.cpp", content);

  std::ifstream created_file(filename.c_str());
  std::string inserted_content((std::istreambuf_iterator<char>(created_file)),
                               (std::istreambuf_iterator<char>()));
  created_file.close();
  EXPECT_EQ(initial_content+content, inserted_content);

  // Add code to specific position
  content = "return 0;";
  repo->addCode("temp.cpp", content, 2, 13);

  std::ifstream nonempty_file(filename.c_str());
  std::string new_content((std::istreambuf_iterator<char>(nonempty_file)),
                          (std::istreambuf_iterator<char>()));
  nonempty_file.close();
  EXPECT_EQ(initial_content+"int main() {return 0;}\n", new_content);
}

// Action: add code to a position with line/col number of out of bound
// (i.e. larger than EOF, negative, larger than line size)
// Expected Output: add code to the nearest valid location in target file.
TEST_F(GitHarnessTest, addCode_PositionOutOfBound) {
  repo->initiateGitRepo(repo_name);
  std::string filename = repo_name + "/" + "temp.cpp";
  std::string initial_content = "int main() {}\n//comment\n";
  repo->addFile("temp.cpp", initial_content);

  // negative line and column number
  // code is added to end (col <= 0) of last line (line <= 0)
  std::string content = "//add\n";
  repo->addCode("temp.cpp", content, -100, -200);

  std::ifstream created_file(filename.c_str());
  std::string inserted_content((std::istreambuf_iterator<char>(created_file)),
                               (std::istreambuf_iterator<char>()));
  created_file.close();
  EXPECT_EQ("int main() {}\n//comment\n//add\n", inserted_content);

  // too large line and column number
  // code is added to end (col >> line size) of last line (line >> file size)
  repo->addCode("temp.cpp", content, 100, 100);

  std::ifstream file(filename.c_str());
  std::string new_content((std::istreambuf_iterator<char>(file)),
                          (std::istreambuf_iterator<char>()));
  file.close();
  EXPECT_EQ("int main() {}\n//comment\n//add\n//add\n", new_content);
}

// Action: add code to nonexisted file
// Expected Output: assertion error
TEST_F(GitHarnessTest, addCode_FileNotExists) {
  repo->initiateGitRepo(repo_name);
  std::string content = "content";
  EXPECT_DEATH({repo->addCode("temp.cpp", content);}, "Assertion.*fileExists");
}

// Action: delete code lines within file length
// Expected Output: target lines are deleted from target file
TEST_F(GitHarnessTest, deleteCode_Normal) {
  repo->initiateGitRepo(repo_name);
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  std::vector<int> target_lines{2, 4};
  repo->addFile("temp.cpp", initial_content)
      .deleteCode("temp.cpp", target_lines);

  std::string filename = repo_name + "/" + "temp.cpp";
  std::ifstream file(filename.c_str());
  std::string content((std::istreambuf_iterator<char>(file)),
                               (std::istreambuf_iterator<char>()));
  file.close();
  EXPECT_EQ("int main() {\n}\n", content);
}

// Action: delete code lines out of file range (negative, or too large)
// Expected Output: no lines are deleted
TEST_F(GitHarnessTest, deleteCode_TargetLineOutOfBound) {
  repo->initiateGitRepo(repo_name);
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  std::vector<int> target_lines{-2, 400};
  repo->addFile("temp.cpp", initial_content)
      .deleteCode("temp.cpp", target_lines);

  std::string filename = repo_name + "/" + "temp.cpp";
  std::ifstream file(filename.c_str());
  std::string content((std::istreambuf_iterator<char>(file)),
                               (std::istreambuf_iterator<char>()));
  file.close();
  EXPECT_EQ(initial_content, content);
}

// Action: add code to nonexisted file
// Expected Output: assertion error
TEST_F(GitHarnessTest, deleteCode_FileNotExists) {
  repo->initiateGitRepo(repo_name);
  std::vector<int> target_lines{2};
  EXPECT_DEATH(repo->deleteCode("temp.cpp", target_lines), "Assert.*fileExist");
}

// Action: create 2 new files in the git repo and git add 1 of them.
// Expected Output: target file is staged, the other one remains untracked.
TEST_F(GitHarnessTest, stageFile_Normal) {
  // Setup new git repo with 2 new files.
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  repo->initiateGitRepo(repo_name).addFile("temp.cpp", initial_content);
  repo->addFile("temp2.cpp", initial_content);
  std::vector<std::string> target_files{"temp.cpp"};

  // Get list of staged and unstaged files before git add
  git_status_list *status;
  git_status_options statusopt{1};
  statusopt.show  = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
  statusopt.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
                    GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
                    GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;
  libgitErrorCheck(git_status_list_new(&status, repo->getGitRepo(), &statusopt),
                   "Could not get status");
  std::vector<std::string> staged_files_before, unstaged_files_before;
  getStagedAndUnstagedFiles(&staged_files_before,
                            &unstaged_files_before, status);
  git_status_list_free(status);

  repo->stageFile(target_files);

  // Get list of staged and unstaged files after git add
  libgitErrorCheck(git_status_list_new(&status, repo->getGitRepo(), &statusopt),
                   "Could not get status");
  std::vector<std::string> staged_files_after, unstaged_files_after;
  getStagedAndUnstagedFiles(&staged_files_after,
                            &unstaged_files_after, status);
  git_status_list_free(status);

  // Before git add, 2 files are untracked, 0 files are staged.
  // After git add, 1 file is untracked, 1 file is staged
  EXPECT_EQ(staged_files_before.size(), 0);

  EXPECT_EQ(unstaged_files_before.size(), 2);
  EXPECT_EQ(unstaged_files_before[0].compare("temp.cpp"), 0);
  EXPECT_EQ(unstaged_files_before[1].compare("temp2.cpp"), 0);

  EXPECT_EQ(staged_files_after.size(), 1);
  EXPECT_EQ(staged_files_after[0].compare("temp.cpp"), 0);

  EXPECT_EQ(unstaged_files_after.size(), 1);
  EXPECT_EQ(unstaged_files_after[0].compare("temp2.cpp"), 0);
}

// Action: git add a file that does not exist
// Expected Output: assertion error
TEST_F(GitHarnessTest, stageFile_FileNotExist) {
  repo->initiateGitRepo(repo_name);
  std::vector<std::string> target_files{"temp.cpp"};
  EXPECT_DEATH(repo->stageFile(target_files), "Assertion.*fileExists");
}

// Action: create a new file and commit with a message
// Expected Output: HEAD commit and new commit point to the same commit
TEST_F(GitHarnessTest, commit_Normal) {
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  repo->initiateGitRepo(repo_name).addFile("temp.cpp", initial_content);
  std::vector<std::string> target_files{"temp.cpp"};
  repo->stageFile(target_files);

  std::string message{"test commit_Normal"};
  repo->commit(message);

  git_oid head_oid;
  git_commit *head_commit, *commit;
  git_oid latest_oid = repo->getLatestCommitId();

  EXPECT_EQ(git_reference_name_to_id(&head_oid, repo->getGitRepo(), "HEAD"), 0);
  EXPECT_EQ(git_commit_lookup(&head_commit, repo->getGitRepo(), &head_oid), 0);
  EXPECT_EQ(git_commit_lookup(&commit, repo->getGitRepo(), &latest_oid), 0);
  EXPECT_EQ(head_commit, commit);
}

// Action: tag an untagged commit
// Expected Output: target commit is tagged
TEST_F(GitHarnessTest, addTagLightweight_Normal) {
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  std::vector<std::string> target_files{"temp.cpp"};
  std::string message{"test commit_Normal"};

  repo->initiateGitRepo(repo_name).addFile("temp.cpp", initial_content)
      .stageFile(target_files).commit(message);

  std::string tag_name{"v0.1"};
  git_oid target_oid{repo->getLatestCommitId()};
  repo->addTagLightweight(tag_name, target_oid);

  git_object *obj;
  git_reference *ref;

  git_repository *git_repo = repo->getGitRepo();
  EXPECT_EQ(git_revparse_ext(&obj, &ref, git_repo, tag_name.c_str()), 0);
  const git_oid *tagged_oid = git_reference_target(ref);

  // commid id used during tag and the commit id referenced by the tag are same.
  EXPECT_TRUE(tagged_oid != nullptr);
  EXPECT_EQ(git_oid_cmp(&target_oid, tagged_oid), 0);
}

// Action: tagging another commit using a used tag.
// Expected Output: error message
TEST_F(GitHarnessTest, addTagLightweight_TaggedCommit) {
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  std::string message{"test commit_Normal"};
  std::string tag_name{"v0.1"};
  std::vector<std::string> target_files{"temp.cpp"};

  // Create and tag the first commit.
  repo->initiateGitRepo(repo_name).addFile("temp.cpp", initial_content)
      .stageFile(target_files).commit(message);
  git_oid target_oid{repo->getLatestCommitId()};
  repo->addTagLightweight(tag_name, target_oid);

  // Create and tag the second commit using the same tag name.
  target_files.clear();
  target_files.push_back("temp2.cpp");
  repo->addFile("temp2.cpp", initial_content).stageFile(target_files)
      .commit(message);
  target_oid = repo->getLatestCommitId();
  EXPECT_DEATH(repo->addTagLightweight(tag_name, target_oid),
               "Unable to create lightweight tag");
}
