/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include <vector>
#include "harness/git-harness/GitHarness.hpp"
#include "helper/TestTempDir.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class GitRepositoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mRepoName = testTempDir("SENTINEL_GITREPOSITORYTEST_TMP_DIR");
    fs::remove_all(mRepoName);
    mRepo = std::make_shared<GitHarness>(mRepoName);
    mRepoName = fs::canonical(mRepoName);
  }

  void TearDown() override {
    if (!HasFailure()) {
      fs::remove_all(mRepoName);
    }
  }

  fs::path mRepoName;
  std::shared_ptr<GitHarness> mRepo;
};

TEST_F(GitRepositoryTest, testInvalidRepositoryThrow) {
  // Use a directory that is NOT inside any git repository so that
  // git_repository_open_ext (which now searches parent directories) still fails.
  fs::path nonGitPath = testTempDir("SENTINEL_NON_GIT_DIR");
  fs::remove_all(nonGitPath);
  fs::create_directories(nonGitPath);

  EXPECT_THROW(
      {
        GitRepository gitRepo(nonGitPath.string());
        SourceLines lines = gitRepo.getSourceLines("commit");
      },
      RepositoryException);

  fs::remove_all(nonGitPath);
}

TEST_F(GitRepositoryTest, testSubdirectoryOfRepoWorks) {
  // A subdirectory of a git repo should be accepted — git_repository_open_ext
  // searches parent directories, so the .git at mRepoName is found.
  fs::path subdir = mRepoName / "subdir_for_test";
  fs::create_directories(subdir);

  EXPECT_NO_THROW({
    GitRepository gitRepo(subdir.string());
    gitRepo.getSourceLines("all");
  });
}

TEST_F(GitRepositoryTest, testPatterns) {
  std::string tmpPath = mRepoName / "test";
  fs::create_directories(tmpPath);

  GitRepository gitRepo(mRepoName, {}, {"file.txt", "file?.txt", "/tmp/", "*/test/*"});
  EXPECT_NO_THROW(gitRepo.getSourceLines("all"));
}

TEST_F(GitRepositoryTest, testNegationPatterns) {
  std::string tmpPath = mRepoName / "test";
  fs::create_directories(tmpPath);

  GitRepository gitRepo(mRepoName, {}, {"!*.c", "!file?.txt", "!data/*.csv", "!*/test/*"});
  EXPECT_NO_THROW(gitRepo.getSourceLines("all"));
}

TEST_F(GitRepositoryTest, testGetSourceLinesWithNoCommit) {
  GitRepository gitRepo(mRepoName);
  std::string content = "int main() {\n}\n";
  std::vector<std::string> stageFiles{"temp.cpp"};

  // nothing
  {
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_TRUE(sourceLines.empty());
  }
  // only workdir
  {
    mRepo->addFile(stageFiles[0], content);
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_TRUE(sourceLines.empty());
  }
  // only index
  {
    mRepo->stageFile(stageFiles);
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_EQ(sourceLines.size(), 2);
  }
}

TEST_F(GitRepositoryTest, testGetSourceLinesWithSingleCommit) {
  GitRepository gitRepo(mRepoName);
  std::string content = "int main() {\n}\n";
  std::vector<std::string> stageFiles{"temp.cpp"};

  mRepo->addFile(stageFiles[0], content);
  mRepo->stageFile(stageFiles);
  mRepo->commit("init");
  // nothing
  {
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_EQ(sourceLines.size(), 2);
  }
  // only workdir
  {
    mRepo->addCode(stageFiles[0], "int a = 1; a = a + 2;\n", 2, 1);
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_EQ(sourceLines.size(), 3);
  }
  // only index
  {
    mRepo->stageFile(stageFiles);
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_EQ(sourceLines.size(), 3);
  }
}

TEST_F(GitRepositoryTest, testGetSourceLines) {
  GitRepository gitRepo(mRepoName);
  std::string content = "int main() {\n}\n";
  std::vector<std::string> stageFiles{"temp.cpp"};
  std::string stageFilename = mRepoName / "temp.cpp";

  mRepo->addFile(stageFiles[0], content);
  mRepo->stageFile(stageFiles);
  mRepo->commit("init");
  mRepo->addCode(stageFiles[0], "int a = 1; a = a + 2;\n", 2, 1);
  mRepo->stageFile(stageFiles);
  mRepo->commit("insert plus line");

  // nothing
  {
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_EQ(sourceLines.size(), 1);
    EXPECT_EQ(std::count(sourceLines.begin(), sourceLines.end(), SourceLine(stageFilename, 2)), 1);
  }
  // only workdir
  {
    mRepo->addCode(stageFiles[0], "int b = 2; b = a + 4;\n", 3, 1);
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_EQ(sourceLines.size(), 2);
    EXPECT_EQ(std::count(sourceLines.begin(), sourceLines.end(), SourceLine(stageFilename, 3)), 1);
  }
  // only index
  {
    mRepo->stageFile(stageFiles);
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_EQ(sourceLines.size(), 2);
    EXPECT_EQ(std::count(sourceLines.begin(), sourceLines.end(), SourceLine(stageFilename, 3)), 1);
  }
}

TEST_F(GitRepositoryTest, testGetSourceLinesAll) {
  GitRepository gitRepo(mRepoName);
  std::string content = "int main() {\n}\n";
  std::vector<std::string> stageFiles{"temp.cpp"};

  mRepo->addFile(stageFiles[0], content);
  mRepo->stageFile(stageFiles);
  mRepo->commit("init");
  mRepo->addCode(stageFiles[0], "int a = 1; a = a + 2;\n", 2, 1);
  mRepo->stageFile(stageFiles);
  mRepo->commit("insert plus line");

  // nothing
  {
    SourceLines sourceLines = gitRepo.getSourceLines("all");

    EXPECT_EQ(sourceLines.size(), 3);
  }
  // only workdir
  {
    mRepo->addCode(stageFiles[0], "int b = 2; b = a + 4;\n", 3, 1);
    SourceLines sourceLines = gitRepo.getSourceLines("all");

    EXPECT_EQ(sourceLines.size(), 4);
  }
  // only index
  {
    mRepo->stageFile(stageFiles);
    SourceLines sourceLines = gitRepo.getSourceLines("all");

    EXPECT_EQ(sourceLines.size(), 4);
  }
}

TEST_F(GitRepositoryTest, testGetSourceLinesWithDevtoolTag) {
  GitRepository gitRepo(mRepoName);
  std::string content = "int main() {\n}\n";
  std::vector<std::string> stageFiles{"temp.cpp"};

  mRepo->addFile(stageFiles[0], content);
  mRepo->stageFile(stageFiles);
  mRepo->commit("init");
  mRepo->addCode(stageFiles[0], "int a = 1; a = a + 2;\n", 2, 1);
  mRepo->stageFile(stageFiles);
  mRepo->commit("insert plus line");
  mRepo->addTagLightweight("devtool-base");
  mRepo->addCode(stageFiles[0], "int b = 2; b = a + 4;\n", 3, 1);
  mRepo->stageFile(stageFiles);
  mRepo->commit("insert second line");
  SourceLines sourceLines = gitRepo.getSourceLines("commit");

  EXPECT_EQ(sourceLines.size(), 2);
}

TEST_F(GitRepositoryTest, testGetSourceLinesWithMultipleParentCommit) {
  GitRepository gitRepo(mRepoName);
  std::string content = "int main() {\n}\n";
  std::vector<std::string> stageFiles{"temp.cpp"};

  mRepo->addFile(stageFiles[0], content);
  mRepo->stageFile(stageFiles);
  mRepo->commit("init");

  // branch work
  std::vector<std::string> targetBranches{"sub"};
  std::vector<std::string> subStageFiles{"test.cpp"};
  mRepo->createBranch(targetBranches[0]);
  mRepo->addFile(subStageFiles[0], "int add(int a, int b)\n{\n}\n");
  mRepo->stageFile(subStageFiles);
  mRepo->commit("add test.cpp");
  mRepo->addCode(subStageFiles[0], "    return a + b;\n", 3, 1);
  mRepo->stageFile(subStageFiles);
  mRepo->commit("insert add function");

  // master work
  mRepo->checkoutBranch("master");
  mRepo->addCode(stageFiles[0], "int a = 1; a = a + 2;\n", 2, 1);
  mRepo->stageFile(stageFiles);
  mRepo->commit("insert plus line");

  mRepo->merge(targetBranches);

  SourceLines sourceLines = gitRepo.getSourceLines("commit");
  EXPECT_EQ(sourceLines.size(), 4);
}

TEST_F(GitRepositoryTest, testIsTarget) {
  std::string content = "int main() {\n}\n";
  std::vector<std::string> stageFiles{"temp.cpp", "temp.c", "test/test.cpp"};

  mRepo->addFolder("test");

  for (auto& file : stageFiles) {
    mRepo->addFile(file, content);
  }
  GitRepository gitRepo(mRepoName, {"cpp", "hpp"}, {"!test/*"});
  EXPECT_TRUE(gitRepo.isTargetPath("temp.cpp"));
  EXPECT_FALSE(gitRepo.isTargetPath("temp.c"));
  EXPECT_FALSE(gitRepo.isTargetPath("test/test.cpp"));
}

TEST_F(GitRepositoryTest, testMixedIncludeAndExcludePatterns) {
  mRepo->addFolder("src");
  mRepo->addFolder("src/util");
  mRepo->addFolder("src/test");
  mRepo->addFile("src/main.cpp", "int main() {}\n");
  mRepo->addFile("src/util/helper.cpp", "void help() {}\n");
  mRepo->addFile("src/test/mock.cpp", "void mock() {}\n");
  mRepo->stageFile({"src/main.cpp", "src/util/helper.cpp", "src/test/mock.cpp"});
  mRepo->commit("add files");

  GitRepository gitRepo(mRepoName, {"cpp"}, {"src/**", "!src/test/*"});
  SourceLines lines = gitRepo.getSourceLines("all");

  bool hasMain = false;
  bool hasHelper = false;
  bool hasMock = false;
  for (const auto& sl : lines) {
    std::string p = sl.getPath().string();
    if (p.find("main.cpp") != std::string::npos) hasMain = true;
    if (p.find("helper.cpp") != std::string::npos) hasHelper = true;
    if (p.find("mock.cpp") != std::string::npos) hasMock = true;
  }
  EXPECT_TRUE(hasMain);
  EXPECT_TRUE(hasHelper);
  EXPECT_FALSE(hasMock);
}

// ---------------------------------------------------------------------------
// Multi-repo tests (Android-style workspace with multiple nested git repos)
// ---------------------------------------------------------------------------

TEST_F(GitRepositoryTest, testGetSourceLinesWithMultipleNestedRepos) {
  // Create a workspace directory that is NOT itself a git repo.
  // Two nested git repos live inside it (simulates an Android repo workspace).
  fs::path multiRoot = testTempDir("SENTINEL_MULTIREPO");
  fs::remove_all(multiRoot);
  fs::create_directories(multiRoot);

  fs::path compA = multiRoot / "comp_a";
  fs::path compB = multiRoot / "comp_b";

  // Repo A: two commits so "commit" scope yields the delta of the second commit.
  auto repoA = std::make_shared<GitHarness>(compA);
  repoA->addFile("a.cpp", "int funcA() {\n    return 1;\n}\n");
  repoA->stageFile({"a.cpp"});
  repoA->commit("init A");
  repoA->addCode("a.cpp", "int extra = 42;\n", 3, 1);
  repoA->stageFile({"a.cpp"});
  repoA->commit("add extra");

  // Repo B: same pattern.
  auto repoB = std::make_shared<GitHarness>(compB);
  repoB->addFile("b.cpp", "int funcB() {\n    return 2;\n}\n");
  repoB->stageFile({"b.cpp"});
  repoB->commit("init B");
  repoB->addCode("b.cpp", "int extra2 = 99;\n", 3, 1);
  repoB->stageFile({"b.cpp"});
  repoB->commit("add extra2");

  // multiRoot has no .git; getSourceLines must still find both nested repos.
  GitRepository gitRepo(multiRoot);
  SourceLines sourceLines = gitRepo.getSourceLines("commit");

  // Each repo contributes exactly 1 changed line (the inserted line 3).
  EXPECT_EQ(sourceLines.size(), 2u);

  fs::path fileA = fs::canonical(compA / "a.cpp");
  fs::path fileB = fs::canonical(compB / "b.cpp");
  EXPECT_EQ(std::count(sourceLines.begin(), sourceLines.end(), SourceLine(fileA, 3)), 1);
  EXPECT_EQ(std::count(sourceLines.begin(), sourceLines.end(), SourceLine(fileB, 3)), 1);
  EXPECT_TRUE(std::is_sorted(sourceLines.begin(), sourceLines.end()));

  fs::remove_all(multiRoot);
}

TEST_F(GitRepositoryTest, testGetSourceLinesWithMultipleNestedReposAllScope) {
  // Same workspace layout, scope "all" — every line of every tracked file is returned.
  fs::path multiRoot = testTempDir("SENTINEL_MULTIREPO_ALL");
  fs::remove_all(multiRoot);
  fs::create_directories(multiRoot);

  fs::path compA = multiRoot / "comp_a";
  fs::path compB = multiRoot / "comp_b";

  auto repoA = std::make_shared<GitHarness>(compA);
  repoA->addFile("a.cpp", "int funcA() {\n    return 1;\n}\n");
  repoA->stageFile({"a.cpp"});
  repoA->commit("init A");

  auto repoB = std::make_shared<GitHarness>(compB);
  repoB->addFile("b.cpp", "int funcB() {\n    return 2;\n}\n");
  repoB->stageFile({"b.cpp"});
  repoB->commit("init B");

  GitRepository gitRepo(multiRoot);
  SourceLines sourceLines = gitRepo.getSourceLines("all");

  // a.cpp has 3 lines, b.cpp has 3 lines → 6 total.
  EXPECT_EQ(sourceLines.size(), 6u);

  // Lines from both files must be present.
  fs::path fileA = fs::canonical(compA / "a.cpp");
  fs::path fileB = fs::canonical(compB / "b.cpp");
  EXPECT_GT(std::count_if(sourceLines.begin(), sourceLines.end(),
                          [&](const SourceLine& sl) { return sl.getPath() == fileA; }),
            0);
  EXPECT_GT(std::count_if(sourceLines.begin(), sourceLines.end(),
                          [&](const SourceLine& sl) { return sl.getPath() == fileB; }),
            0);
  EXPECT_TRUE(std::is_sorted(sourceLines.begin(), sourceLines.end()));

  fs::remove_all(multiRoot);
}

TEST_F(GitRepositoryTest, testSourceRootFilterExcludesSiblingDirs) {
  // sourceRoot is a subdirectory of the single git repo.
  // Files in sibling directories of sourceRoot must NOT appear in source lines.
  fs::path subdir = mRepoName / "target_subdir";
  fs::path sibling = mRepoName / "sibling_subdir";
  fs::create_directories(subdir);
  fs::create_directories(sibling);

  mRepo->addFile("target_subdir/foo.cpp", "int a = 1;\n");
  mRepo->addFile("sibling_subdir/bar.cpp", "int b = 2;\n");
  mRepo->stageFile({"target_subdir/foo.cpp", "sibling_subdir/bar.cpp"});
  mRepo->commit("add files");

  // Use target_subdir as sourceRoot — sibling_subdir/bar.cpp must be excluded.
  GitRepository gitRepo(subdir);
  SourceLines sourceLines = gitRepo.getSourceLines("all");

  bool hasFoo = false;
  bool hasBar = false;
  for (const auto& sl : sourceLines) {
    std::string p = sl.getPath().string();
    if (p.find("foo.cpp") != std::string::npos) {
      hasFoo = true;
    }
    if (p.find("bar.cpp") != std::string::npos) {
      hasBar = true;
    }
  }
  EXPECT_TRUE(hasFoo);
  EXPECT_FALSE(hasBar);
}

TEST_F(GitRepositoryTest, testAddSkipDirExcludesNestedRepo) {
  // Simulate a workspace directory that contains a nested git repo.
  // Without addSkipDir the nested repo is discovered and contributes source lines.
  // With addSkipDir the workspace is not traversed and those lines are absent.
  fs::path workspaceDir = mRepoName / "workspace";
  fs::path nestedRepoDir = workspaceDir / "nested";

  // GitHarness creates the directory internally; do not pre-create nestedRepoDir.
  auto nestedRepo = std::make_shared<GitHarness>(nestedRepoDir.string());
  nestedRepo->addFile("backup.cpp", "int backup() {\n    return 0;\n}\n");
  nestedRepo->stageFile({"backup.cpp"});
  nestedRepo->commit("nested init");

  // Without skipDir: nested repo's source lines are visible.
  {
    GitRepository gitRepo(mRepoName);
    SourceLines lines = gitRepo.getSourceLines("all");
    bool hasNestedFile = std::any_of(lines.begin(), lines.end(), [&](const SourceLine& sl) {
      return sl.getPath().string().find("workspace") != std::string::npos;
    });
    EXPECT_TRUE(hasNestedFile);
  }

  // With addSkipDir: workspace is not traversed, nested lines absent.
  {
    GitRepository gitRepo(mRepoName);
    gitRepo.addSkipDir(workspaceDir);
    SourceLines lines = gitRepo.getSourceLines("all");
    bool hasNestedFile = std::any_of(lines.begin(), lines.end(), [&](const SourceLine& sl) {
      return sl.getPath().string().find("workspace") != std::string::npos;
    });
    EXPECT_FALSE(hasNestedFile);
  }
}

}  // namespace sentinel
