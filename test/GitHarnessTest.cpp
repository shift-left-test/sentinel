/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <cstdio>
#include <filesystem>  // NOLINT
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "git-harness/GitHarness.hpp"
#include "sentinel/exceptions/IOException.hpp"
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class GitHarnessTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mRepoName = testTempDir("SENTINEL_GITHARNESSTEST_TMP_DIR");
    fs::remove_all(mRepoName);
    mRepo = std::make_shared<GitHarness>(mRepoName);
  }

  void TearDown() override {
    fs::remove_all(mRepoName);
  }

  static void getStagedAndUnstagedFiles(std::vector<std::string>* staged_files,
                                        std::vector<std::string>* unstaged_files, git_status_list* status) {
    std::size_t i;
    std::size_t maxi = git_status_list_entrycount(status);
    const git_status_entry* s;
    const char *old_path, *new_path;

    for (i = 0; i < maxi; ++i) {
      s = git_status_byindex(status, i);

      // A staged file is a file that is added, modified , deleted, renamed, or
      // typechanged in git repo index.
      if (s != nullptr && s->status != GIT_STATUS_CURRENT &&
          ((s->status & GIT_STATUS_INDEX_NEW) != 0 || (s->status & GIT_STATUS_INDEX_MODIFIED) != 0 ||
           (s->status & GIT_STATUS_INDEX_DELETED) != 0 || (s->status & GIT_STATUS_INDEX_RENAMED) != 0 ||
           (s->status & GIT_STATUS_INDEX_TYPECHANGE) != 0)) {
        old_path = s->head_to_index->old_file.path;
        new_path = s->head_to_index->new_file.path;
        staged_files->emplace_back((old_path != nullptr) ? old_path : new_path);
      }

      // An unstaged file is a file that is modified, deleted, renamed or
      // typechanged in git repo worktree
      if (s != nullptr && s->status != GIT_STATUS_CURRENT && s->index_to_workdir != nullptr &&
          ((s->status & GIT_STATUS_WT_MODIFIED) != 0 || (s->status & GIT_STATUS_WT_DELETED) != 0 ||
           (s->status & GIT_STATUS_WT_RENAMED) != 0 || (s->status & GIT_STATUS_WT_TYPECHANGE) != 0)) {
        old_path = s->index_to_workdir->old_file.path;
        new_path = s->index_to_workdir->new_file.path;
        unstaged_files->emplace_back((old_path != nullptr) ? old_path : new_path);
      }

      // An untracked file is a new file in the work tree
      if (s != nullptr && s->status == GIT_STATUS_WT_NEW && s->index_to_workdir != nullptr) {
        old_path = s->index_to_workdir->old_file.path;
        unstaged_files->emplace_back(old_path);
      }
    }
  }

  fs::path mRepoName;
  std::shared_ptr<GitHarness> mRepo;
};

// Action: create a new directory and git init
// Expected Output: new directory created, containing .git folder
TEST_F(GitHarnessTest, testInitWorks) {
  EXPECT_TRUE(fs::is_directory(mRepoName));

  auto git_dir = mRepoName / ".git";
  EXPECT_TRUE(fs::is_directory(git_dir));
}

// Action: initiate a git repo twice
// Expected Output: assertion error
TEST_F(GitHarnessTest, testInitFailWhenDirAlreadyExists) {
  EXPECT_THROW(std::unique_ptr<GitHarness> test_repo = std::make_unique<GitHarness>(mRepoName), std::runtime_error);
}

// Action: create new folder in git repo
// Expected Output: folder created at within repo at correct location
TEST_F(GitHarnessTest, testAddFolderWorks) {
  mRepo->addFolder("temp");
  EXPECT_TRUE(fs::is_directory(mRepoName / "temp"));
}

// Action: call addFolder with a folder that already exist
// Expected Output: assertion errors
TEST_F(GitHarnessTest, testAddFolderFailWhenFolderExisted) {
  mRepo->addFolder("temp");
  EXPECT_THROW({ mRepo->addFolder("temp"); }, std::runtime_error);
}

// Action: create new file in git repo with empty main function
// Expected Output: file created at within repo at correct location and content.
TEST_F(GitHarnessTest, testAddFileWorks) {
  std::string filename = "temp.cpp";
  std::string content = "int main() {}";
  mRepo->addFile(filename, content);

  filename = mRepoName / filename;
  EXPECT_TRUE(fs::is_regular_file(filename));

  EXPECT_EQ(content, testutil::readFile(filename));
}

// Action: call addFile with a file that already exist
// Expected Output: assertion errors
TEST_F(GitHarnessTest, testAddFileFailWhenFileExisted) {
  std::string content = "int main() {}";
  mRepo->addFile("temp.cpp", content);
  EXPECT_THROW({ mRepo->addFile("temp.cpp", content); }, std::runtime_error);
}

// Action: call addFile with invalid path
// Expected Output: assertion errors
TEST_F(GitHarnessTest, testAddFileFailWhenInvalidPathGiven) {
  std::string content = "int main() {}";
  EXPECT_THROW({ mRepo->addFile("unexisted/temp.cpp", content); }, std::runtime_error);
}

// Action: add content to file
// Expected Output: content are added properly to the right file. Other files are unaffected.
TEST_F(GitHarnessTest, testAddCodeWorks) {
  std::string filename = mRepoName / "temp.cpp";
  std::string initial_content = "// comment\n";
  mRepo->addFile("temp.cpp", initial_content);

  // Default behavior: add code to end of file if no position is specified
  std::string content = "int main() {}\n";
  mRepo->addCode("temp.cpp", content);

  EXPECT_EQ(initial_content + content, testutil::readFile(filename));

  // Add code to specific position
  content = "return 0;";
  mRepo->addCode("temp.cpp", content, 2, 13);

  EXPECT_EQ(initial_content + "int main() {return 0;}\n", testutil::readFile(filename));
}

// Action: add code to a position with line/col number of out of bound
// (i.e. larger than EOF, negative, larger than line size)
// Expected Output: add code to the nearest valid location in target file.
TEST_F(GitHarnessTest, testAddCodeFailWhenPositionOutOfBound) {
  std::string filename = mRepoName / "temp.cpp";
  std::string initial_content = "int main() {}\n//comment\n";
  mRepo->addFile("temp.cpp", initial_content);

  // negative line and column number
  // code is added to end (col <= 0) of last line (line <= 0)
  std::string content = "//add\n";
  mRepo->addCode("temp.cpp", content, 0, 0);

  EXPECT_EQ("int main() {}\n//comment\n//add\n", testutil::readFile(filename));

  // too large line and column number
  // code is added to end (col >> line size) of last line (line >> file size)
  mRepo->addCode("temp.cpp", content, 100, 100);

  EXPECT_EQ("int main() {}\n//comment\n//add\n//add\n", testutil::readFile(filename));
}

// Action: add code to nonexistent file
// Expected Output: assertion error
TEST_F(GitHarnessTest, testAddCodeFailWhenFileNotExists) {
  std::string content = "content";
  EXPECT_THROW({ mRepo->addCode("temp.cpp", content); }, std::runtime_error);
}

// Action: delete code lines within file length
// Expected Output: target lines are deleted from target file
TEST_F(GitHarnessTest, testDeleteCodeWorks) {
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  std::vector<std::size_t> target_lines{2, 4};
  mRepo->addFile("temp.cpp", initial_content).deleteCode("temp.cpp", target_lines);

  std::string filename = mRepoName / "temp.cpp";
  EXPECT_EQ("int main() {\n}\n", testutil::readFile(filename));
}

// Action: delete code lines out of file range (negative, or too large)
// Expected Output: no lines are deleted
TEST_F(GitHarnessTest, testDeleteCodeFailWhenTargetLineOutOfBound) {
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  std::vector<std::size_t> target_lines{0, 400};
  mRepo->addFile("temp.cpp", initial_content).deleteCode("temp.cpp", target_lines);

  std::string filename = mRepoName / "temp.cpp";
  EXPECT_EQ(initial_content, testutil::readFile(filename));
}

// Action: add code to nonexistent file
// Expected Output: assertion error
TEST_F(GitHarnessTest, testDeleteCodeFailWhenFileNotExists) {
  std::vector<std::size_t> target_lines{2};
  EXPECT_THROW(mRepo->deleteCode("temp.cpp", target_lines), std::runtime_error);
}

// Action: create 2 new files in the git repo and git add 1 of them.
// Expected Output: target file is staged, the other one remains untracked.
TEST_F(GitHarnessTest, testStageFileWorks) {
  // Setup new git repo with 2 new files.
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  mRepo->addFile("temp.cpp", initial_content);
  mRepo->addFile("temp2.cpp", initial_content);
  std::vector<std::string> target_files{"temp.cpp"};

  // Get list of staged and unstaged files before git add
  git_status_list* status;
  git_status_options statusopt{1};
  statusopt.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
  statusopt.flags =
      GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX | GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;
  GitHarness::libgitErrorCheck(git_status_list_new(&status, mRepo->getGitRepo(), &statusopt), "Could not get status");
  std::vector<std::string> staged_files_before, unstaged_files_before;
  getStagedAndUnstagedFiles(&staged_files_before, &unstaged_files_before, status);
  git_status_list_free(status);

  // stage files: git add
  mRepo->stageFile(target_files);

  // Get list of staged and unstaged files after git add
  GitHarness::libgitErrorCheck(git_status_list_new(&status, mRepo->getGitRepo(), &statusopt), "Could not get status");
  std::vector<std::string> staged_files_after, unstaged_files_after;
  getStagedAndUnstagedFiles(&staged_files_after, &unstaged_files_after, status);
  git_status_list_free(status);

  // Before git add, 2 files are untracked, 0 files are staged.
  EXPECT_EQ(staged_files_before.size(), 0);
  EXPECT_EQ(unstaged_files_before.size(), 2);
  EXPECT_EQ(unstaged_files_before[0].compare("temp.cpp"), 0);
  EXPECT_EQ(unstaged_files_before[1].compare("temp2.cpp"), 0);

  // After git add, 1 file is untracked, 1 file is staged
  EXPECT_EQ(staged_files_after.size(), 1);
  EXPECT_EQ(staged_files_after[0].compare("temp.cpp"), 0);
  EXPECT_EQ(unstaged_files_after.size(), 1);
  EXPECT_EQ(unstaged_files_after[0].compare("temp2.cpp"), 0);
}

// Action: git add a file that does not exist
// Expected Output: assertion error
TEST_F(GitHarnessTest, testStageFileFailWhenFileNotExist) {
  std::vector<std::string> target_files{"temp.cpp"};
  EXPECT_THROW(mRepo->stageFile(target_files), std::runtime_error);
}

// Action: create a new file and commit with a message
// Expected Output: HEAD commit and new commit point to the same commit
TEST_F(GitHarnessTest, testCommitWorks) {
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  mRepo->addFile("temp.cpp", initial_content);
  std::vector<std::string> target_files{"temp.cpp"};
  mRepo->stageFile(target_files);

  std::string message{"test commit_Normal"};
  mRepo->commit(message);

  git_oid head_oid, latest_oid;
  git_commit *head_commit, *commit;
  std::string latest_oid_str = mRepo->getLatestCommitId();
  GitHarness::libgitErrorCheck(git_oid_fromstrp(&latest_oid, latest_oid_str.c_str()), "Error retrieving commit");

  EXPECT_EQ(git_reference_name_to_id(&head_oid, mRepo->getGitRepo(), "HEAD"), 0);
  EXPECT_EQ(git_commit_lookup(&head_commit, mRepo->getGitRepo(), &head_oid), 0);
  EXPECT_EQ(git_commit_lookup(&commit, mRepo->getGitRepo(), &latest_oid), 0);
  EXPECT_EQ(head_commit, commit);

  git_commit_free(head_commit);
  git_commit_free(commit);
}

TEST_F(GitHarnessTest, testGetLatestCommitFailWhenNoCommitWasMade) {
  EXPECT_THROW(mRepo->getLatestCommitId(), std::range_error);
}

// Action: tag an untagged commit
// Expected Output: target commit is tagged
TEST_F(GitHarnessTest, testAddTagLightweightWorks) {
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  std::vector<std::string> target_files{"temp.cpp"};
  std::string message{"test commit_Normal"};

  mRepo->addFile("temp.cpp", initial_content).stageFile(target_files).commit(message);

  std::string tag_name{"v0.1"};
  std::string target_oid_str{mRepo->getLatestCommitId()};
  mRepo->addTagLightweight(tag_name, target_oid_str);

  git_object* obj;
  git_reference* ref;

  git_repository* git_repo = mRepo->getGitRepo();
  EXPECT_EQ(git_revparse_ext(&obj, &ref, git_repo, tag_name.c_str()), 0);
  const git_oid* tagged_oid = git_reference_target(ref);
  git_oid target_oid;
  git_oid_fromstrp(&target_oid, target_oid_str.c_str());

  // commit id used during tag and the commit id referenced by the tag are same.
  EXPECT_TRUE(tagged_oid != nullptr);
  EXPECT_EQ(git_oid_cmp(&target_oid, tagged_oid), 0);

  git_object_free(obj);
  git_reference_free(ref);
}

// Action: tagging another commit using a used tag.
// Expected Output: error message
TEST_F(GitHarnessTest, testAddTagLightweightFailWhenCommitAlreadyTagged) {
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  std::string message{"test commit_Normal"};
  std::string tag_name{"v0.1"};
  std::vector<std::string> target_files{"temp.cpp"};

  // Create and tag the first commit.
  mRepo->addFile("temp.cpp", initial_content).stageFile(target_files).commit(message);
  std::string target_oid{mRepo->getLatestCommitId()};
  EXPECT_THROW(mRepo->addTagLightweight("", target_oid), std::runtime_error);
  mRepo->addTagLightweight(tag_name, target_oid);

  // Create and tag the second commit using the same tag name.
  target_files.clear();
  target_files.emplace_back("temp2.cpp");
  mRepo->addFile("temp2.cpp", initial_content).stageFile(target_files).commit(message);
  target_oid = mRepo->getLatestCommitId();
  std::string error_msg = "Unable to create lightweight tag";
  EXPECT_THROW(mRepo->addTagLightweight(tag_name, target_oid), IOException);
}

TEST_F(GitHarnessTest, testCreateBranchWorks) {
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  std::vector<std::string> target_files{"temp.cpp"};
  std::string init_message{"init commit"};

  mRepo->addFile("temp.cpp", initial_content);
  mRepo->stageFile(target_files).commit(init_message);

  std::string branch_name{"b1"};
  mRepo->createBranch(branch_name);

  // Check if branch b1 exists
  git_reference* ref = nullptr;
  EXPECT_EQ(0, git_branch_lookup(&ref, mRepo->getGitRepo(), branch_name.c_str(), GIT_BRANCH_LOCAL));
  git_reference_free(ref);
}

TEST_F(GitHarnessTest, testCheckoutBranchWorks) {
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  std::vector<std::string> target_files{"temp.cpp"};
  std::string init_message{"init commit"};

  mRepo->addFile("temp.cpp", initial_content);
  mRepo->stageFile(target_files).commit(init_message);

  std::string branch_name{"b1"};
  mRepo->createBranch(branch_name);

  std::vector<std::string> target_files2{"temp2.cpp"};
  std::string b1_message{"b1 commit"};

  mRepo->addFile("temp2.cpp", initial_content);
  mRepo->stageFile(target_files2).commit(b1_message);

  // After checkout, HEAD and master should point to the same commit.
  // The repo should only contains temp.cpp and not temp2.cpp
  mRepo->checkoutBranch("master");
  git_reference* master_branch = nullptr;
  git_branch_lookup(&master_branch, mRepo->getGitRepo(), "master", GIT_BRANCH_LOCAL);
  git_oid head_oid;
  git_reference_name_to_id(&head_oid, mRepo->getGitRepo(), "HEAD");
  EXPECT_EQ(git_oid_cmp(git_reference_target(master_branch), &head_oid), 0);
  EXPECT_TRUE(fs::exists(mRepoName / "temp.cpp"));
  EXPECT_FALSE(fs::exists(mRepoName / "temp2.cpp"));

  git_reference_free(master_branch);
}

TEST_F(GitHarnessTest, testMergeWorks) {
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  std::vector<std::string> target_files{"temp.cpp"};
  std::string init_message{"init commit"};

  mRepo->addFile("temp.cpp", initial_content);
  mRepo->stageFile(target_files).commit(init_message);

  // Create branch b1 and add a new file.
  std::vector<std::string> target_files1{"temp1.cpp"};
  std::string b1_message{"b1: add temp1.cpp"};
  mRepo->createBranch("b1");
  mRepo->addFile("temp1.cpp", initial_content);
  mRepo->stageFile(target_files1).commit(b1_message);

  // Create branch b2 and add a new file.
  std::vector<std::string> target_files2{"temp2.cpp"};
  std::string b2_message{"b2: add temp2.cpp"};
  mRepo->checkoutBranch("master");
  mRepo->createBranch("b2");
  mRepo->addFile("temp2.cpp", initial_content);
  mRepo->stageFile(target_files2).commit(b2_message);

  // Add a new file in master branch
  mRepo->checkoutBranch("master");
  std::vector<std::string> target_files3{"temp3.cpp"};
  std::string master_message{"master: add temp3.cpp"};
  mRepo->addFile("temp3.cpp", initial_content);
  mRepo->stageFile(target_files3).commit(master_message);

  std::vector<std::string> target_branches{"b1", "b2"};
  mRepo->merge(target_branches);

  // After merge, HEAD and master should point to the same commit.
  // The repo should contain both temp.cpp and temp2.cpp
  mRepo->checkoutBranch("master");
  git_reference* master_branch = nullptr;
  git_branch_lookup(&master_branch, mRepo->getGitRepo(), "master", GIT_BRANCH_LOCAL);
  git_oid head_oid;
  git_reference_name_to_id(&head_oid, mRepo->getGitRepo(), "HEAD");
  EXPECT_EQ(git_oid_cmp(git_reference_target(master_branch), &head_oid), 0);
  EXPECT_TRUE(fs::exists(mRepoName / "temp.cpp"));
  EXPECT_TRUE(fs::exists(mRepoName / "temp1.cpp"));
  EXPECT_TRUE(fs::exists(mRepoName / "temp2.cpp"));
  EXPECT_TRUE(fs::exists(mRepoName / "temp3.cpp"));
}

TEST_F(GitHarnessTest, testMergeWorksWhenUsingVariadicArguments) {
  std::string initial_content = "int main() {\n\treturn 0;\n}\n//comment\n";
  std::vector<std::string> target_files{"temp.cpp"};
  std::string init_message{"init commit"};

  mRepo->addFile("temp.cpp", initial_content);
  mRepo->stageFile(target_files).commit(init_message);

  // Create branch b1 and add a new file.
  std::vector<std::string> target_files1{"temp1.cpp"};
  std::string b1_message{"b1: add temp1.cpp"};
  mRepo->createBranch("b1");
  mRepo->addFile("temp1.cpp", initial_content);
  mRepo->stageFile(target_files1).commit(b1_message);

  // Create branch b2 and add a new file.
  std::vector<std::string> target_files2{"temp2.cpp"};
  std::string b2_message{"b2: add temp2.cpp"};
  mRepo->checkoutBranch("master");
  mRepo->createBranch("b2");
  mRepo->addFile("temp2.cpp", initial_content);
  mRepo->stageFile(target_files2).commit(b2_message);

  // Add a new file in master branch
  mRepo->checkoutBranch("master");
  std::vector<std::string> target_files3{"temp3.cpp"};
  std::string master_message{"master: add temp3.cpp"};
  mRepo->addFile("temp3.cpp", initial_content);
  mRepo->stageFile(target_files3).commit(master_message);

  mRepo->merge("b1", "b2");

  // After merge, HEAD and master should point to the same commit.
  // The repo should contain both temp.cpp and temp2.cpp
  mRepo->checkoutBranch("master");
  git_reference* master_branch = nullptr;
  git_branch_lookup(&master_branch, mRepo->getGitRepo(), "master", GIT_BRANCH_LOCAL);
  git_oid head_oid;
  git_reference_name_to_id(&head_oid, mRepo->getGitRepo(), "HEAD");
  EXPECT_EQ(git_oid_cmp(git_reference_target(master_branch), &head_oid), 0);
  EXPECT_TRUE(fs::exists(mRepoName / "temp.cpp"));
  EXPECT_TRUE(fs::exists(mRepoName / "temp1.cpp"));
  EXPECT_TRUE(fs::exists(mRepoName / "temp2.cpp"));
  EXPECT_TRUE(fs::exists(mRepoName / "temp3.cpp"));
}

}  // namespace sentinel
