/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include "sentinel/GitRepository.hpp"
#include "harness/git-harness/GitHarness.hpp"
#include "helper/CaptureHelper.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class GitRepositoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    repo_name = fs::temp_directory_path() / "SENTINEL_GITREPOSITORYTEST_TMP_DIR";
    fs::remove_all(repo_name);
    repo = std::make_shared<GitHarness>(repo_name);
    repo_name = fs::canonical(repo_name);

    mStderrCapture = CaptureHelper::getStderrCapture();
    mStdoutCapture = CaptureHelper::getStdoutCapture();
  }

  void TearDown() override {
    if (!HasFailure()) {
      fs::remove_all(repo_name);
    }
  }

  void captureStderr() {
    mStderrCapture->capture();
  }

  std::string capturedStderr() {
    return mStderrCapture->release();
  }

  void captureStdout() {
    mStdoutCapture->capture();
  }

  std::string capturedStdout() {
    return mStdoutCapture->release();
  }

  fs::path repo_name;
  std::shared_ptr<GitHarness> repo;

 private:
  std::shared_ptr<CaptureHelper> mStderrCapture;
  std::shared_ptr<CaptureHelper> mStdoutCapture;
};

TEST_F(GitRepositoryTest, testInvalidRepositoryThrow) {
  // Use a directory that is NOT inside any git repository so that
  // git_repository_open_ext (which now searches parent directories) still fails.
  fs::path nonGitPath = fs::temp_directory_path() / "SENTINEL_NON_GIT_DIR_XYZXYZ";
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
  // searches parent directories, so the .git at repo_name is found.
  fs::path subdir = repo_name / "subdir_for_test";
  fs::create_directories(subdir);

  EXPECT_NO_THROW({
    GitRepository gitRepo(subdir.string());
    gitRepo.getSourceLines("all");
  });
}

TEST_F(GitRepositoryTest, testPatterns) {
  std::string tmpPath = repo_name / "test";
  fs::create_directories(tmpPath);

  Logger::getLogger("GitRepository")->setLevel(Logger::Level::DEBUG);
  captureStderr();
  captureStdout();
  GitRepository gitRepo(repo_name, {}, {"file.txt", "file?.txt", "/tmp/", "*/test/*"}, {});
  EXPECT_TRUE(string::contains(capturedStdout(), "patterns: file.txt, file?.txt, /tmp/, */test/*"));
  Logger::getLogger("GitRepository")->setLevel(Logger::Level::OFF);
}

TEST_F(GitRepositoryTest, testExcludes) {
  std::string tmpPath = repo_name / "test";
  fs::create_directories(tmpPath);

  Logger::getLogger("GitRepository")->setLevel(Logger::Level::DEBUG);
  captureStderr();
  captureStdout();
  GitRepository gitRepo(repo_name, {}, {}, {"*.c", "file?.txt", "data/*.csv", "*/test/*", "*"});
  EXPECT_TRUE(string::contains(capturedStdout(), "excludes: *.c, file?.txt, data/*.csv, */test/*, *"));
  Logger::getLogger("GitRepository")->setLevel(Logger::Level::OFF);
}

TEST_F(GitRepositoryTest, testGetSourceLinesWithNoCommit) {
  GitRepository gitRepo(repo_name);
  std::string content = "int main() {\n}\n";
  std::vector<std::string> stageFiles {"temp.cpp"};

  // nothing
  {
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_TRUE(sourceLines.empty());
  }
  // only workdir
  {
    repo->addFile(stageFiles[0], content);
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_TRUE(sourceLines.empty());
  }
  // only index
  {
    repo->stageFile(stageFiles);
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_EQ(sourceLines.size(), 2);
  }
}

TEST_F(GitRepositoryTest, testGetSourceLinesWithSingleCommit) {
  GitRepository gitRepo(repo_name);
  std::string content = "int main() {\n}\n";
  std::vector<std::string> stageFiles {"temp.cpp"};

  repo->addFile(stageFiles[0], content);
  repo->stageFile(stageFiles);
  repo->commit("init");
  // nothing
  {
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_EQ(sourceLines.size(), 2);
  }
  // only workdir
  {
    repo->addCode(stageFiles[0], "int a = 1; a = a + 2;\n", 2, 1);
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_EQ(sourceLines.size(), 3);
  }
  // only index
  {
    repo->stageFile(stageFiles);
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_EQ(sourceLines.size(), 3);
  }
}

TEST_F(GitRepositoryTest, testGetSourceLines) {
  GitRepository gitRepo(repo_name);
  std::string content = "int main() {\n}\n";
  std::vector<std::string> stageFiles {"temp.cpp"};
  std::string stageFilename = repo_name / "temp.cpp";

  repo->addFile(stageFiles[0], content);
  repo->stageFile(stageFiles);
  repo->commit("init");
  repo->addCode(stageFiles[0], "int a = 1; a = a + 2;\n", 2, 1);
  repo->stageFile(stageFiles);
  repo->commit("insert plus line");

  // nothing
  {
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_EQ(sourceLines.size(), 1);
    EXPECT_EQ(std::count(sourceLines.begin(), sourceLines.end(), SourceLine(stageFilename, 2)), 1);
  }
  // only workdir
  {
    repo->addCode(stageFiles[0], "int b = 2; b = a + 4;\n", 3, 1);
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_EQ(sourceLines.size(), 2);
    EXPECT_EQ(std::count(sourceLines.begin(), sourceLines.end(), SourceLine(stageFilename, 3)), 1);
  }
  // only index
  {
    repo->stageFile(stageFiles);
    SourceLines sourceLines = gitRepo.getSourceLines("commit");

    EXPECT_EQ(sourceLines.size(), 2);
    EXPECT_EQ(std::count(sourceLines.begin(), sourceLines.end(), SourceLine(stageFilename, 3)), 1);
  }
}

TEST_F(GitRepositoryTest, testGetSourceLinesAll) {
  GitRepository gitRepo(repo_name);
  std::string content = "int main() {\n}\n";
  std::vector<std::string> stageFiles {"temp.cpp"};

  repo->addFile(stageFiles[0], content);
  repo->stageFile(stageFiles);
  repo->commit("init");
  repo->addCode(stageFiles[0], "int a = 1; a = a + 2;\n", 2, 1);
  repo->stageFile(stageFiles);
  repo->commit("insert plus line");

  // nothing
  {
    SourceLines sourceLines = gitRepo.getSourceLines("all");

    EXPECT_EQ(sourceLines.size(), 3);
  }
  // only workdir
  {
    repo->addCode(stageFiles[0], "int b = 2; b = a + 4;\n", 3, 1);
    SourceLines sourceLines = gitRepo.getSourceLines("all");

    EXPECT_EQ(sourceLines.size(), 4);
  }
  // only index
  {
    repo->stageFile(stageFiles);
    SourceLines sourceLines = gitRepo.getSourceLines("all");

    EXPECT_EQ(sourceLines.size(), 4);
  }
}

TEST_F(GitRepositoryTest, testGetSourceLinesWithDevtoolTag) {
  GitRepository gitRepo(repo_name);
  std::string content = "int main() {\n}\n";
  std::vector<std::string> stageFiles {"temp.cpp"};

  repo->addFile(stageFiles[0], content);
  repo->stageFile(stageFiles);
  repo->commit("init");
  repo->addCode(stageFiles[0], "int a = 1; a = a + 2;\n", 2, 1);
  repo->stageFile(stageFiles);
  repo->commit("insert plus line");
  repo->addTagLightweight("devtool-base");
  repo->addCode(stageFiles[0], "int b = 2; b = a + 4;\n", 3, 1);
  repo->stageFile(stageFiles);
  repo->commit("insert second line");
  SourceLines sourceLines = gitRepo.getSourceLines("commit");

  EXPECT_EQ(sourceLines.size(), 2);
}

TEST_F(GitRepositoryTest, testGetSourceLinesWithMultipleParentCommit) {
  GitRepository gitRepo(repo_name);
  std::string content = "int main() {\n}\n";
  std::vector<std::string> stageFiles {"temp.cpp"};

  repo->addFile(stageFiles[0], content);
  repo->stageFile(stageFiles);
  repo->commit("init");

  // branch work
  std::vector<std::string> targetBranches {"sub"};
  std::vector<std::string> subStageFiles {"test.cpp"};
  repo->createBranch(targetBranches[0]);
  repo->addFile(subStageFiles[0], "int add(int a, int b)\n{\n}\n");
  repo->stageFile(subStageFiles);
  repo->commit("add test.cpp");
  repo->addCode(subStageFiles[0], "    return a + b;\n", 3, 1);
  repo->stageFile(subStageFiles);
  repo->commit("insert add function");

  // master work
  repo->checkoutBranch("master");
  repo->addCode(stageFiles[0], "int a = 1; a = a + 2;\n", 2, 1);
  repo->stageFile(stageFiles);
  repo->commit("insert plus line");

  repo->merge(targetBranches);

  SourceLines sourceLines = gitRepo.getSourceLines("commit");
  EXPECT_EQ(sourceLines.size(), 4);
}

TEST_F(GitRepositoryTest, testIsTarget) {
  std::string content = "int main() {\n}\n";
  std::vector<std::string> stageFiles {"temp.cpp", "temp.c", "test/test.cpp"};

  repo->addFolder("test");

  for (auto &file : stageFiles) {
    repo->addFile(file, content);
  }
  GitRepository gitRepo(repo_name, {"cpp", "hpp"}, {}, {"*/test/*"});
  EXPECT_TRUE(gitRepo.isTargetPath("temp.cpp"));
  EXPECT_FALSE(gitRepo.isTargetPath("temp.c"));
  EXPECT_FALSE(gitRepo.isTargetPath("test/test.cpp"));
}

// ---------------------------------------------------------------------------
// Multi-repo tests (Android-style workspace with multiple nested git repos)
// ---------------------------------------------------------------------------

TEST_F(GitRepositoryTest, testGetSourceLinesWithMultipleNestedRepos) {
  // Create a workspace directory that is NOT itself a git repo.
  // Two nested git repos live inside it (simulates an Android repo workspace).
  fs::path multiRoot = fs::temp_directory_path() / "SENTINEL_MULTIREPO_TEST_DIR";
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

  fs::remove_all(multiRoot);
}

TEST_F(GitRepositoryTest, testGetSourceLinesWithMultipleNestedReposAllScope) {
  // Same workspace layout, scope "all" — every line of every tracked file is returned.
  fs::path multiRoot = fs::temp_directory_path() / "SENTINEL_MULTIREPO_ALL_TEST_DIR";
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

  fs::remove_all(multiRoot);
}

TEST_F(GitRepositoryTest, testSourceRootFilterExcludesSiblingDirs) {
  // sourceRoot is a subdirectory of the single git repo.
  // Files in sibling directories of sourceRoot must NOT appear in source lines.
  fs::path subdir = repo_name / "target_subdir";
  fs::path sibling = repo_name / "sibling_subdir";
  fs::create_directories(subdir);
  fs::create_directories(sibling);

  repo->addFile("target_subdir/foo.cpp", "int a = 1;\n");
  repo->addFile("sibling_subdir/bar.cpp", "int b = 2;\n");
  repo->stageFile({"target_subdir/foo.cpp", "sibling_subdir/bar.cpp"});
  repo->commit("add files");

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
  fs::path workspaceDir = repo_name / "sentinel_workspace";
  fs::path nestedRepoDir = workspaceDir / "nested";

  // GitHarness creates the directory internally; do not pre-create nestedRepoDir.
  auto nestedRepo = std::make_shared<GitHarness>(nestedRepoDir.string());
  nestedRepo->addFile("backup.cpp", "int backup() {\n    return 0;\n}\n");
  nestedRepo->stageFile({"backup.cpp"});
  nestedRepo->commit("nested init");

  // Without skipDir: nested repo's source lines are visible.
  {
    GitRepository gitRepo(repo_name);
    SourceLines lines = gitRepo.getSourceLines("all");
    bool hasNestedFile = std::any_of(lines.begin(), lines.end(), [&](const SourceLine& sl) {
      return sl.getPath().string().find("sentinel_workspace") != std::string::npos;
    });
    EXPECT_TRUE(hasNestedFile);
  }

  // With addSkipDir: workspace is not traversed, nested lines absent.
  {
    GitRepository gitRepo(repo_name);
    gitRepo.addSkipDir(workspaceDir);
    SourceLines lines = gitRepo.getSourceLines("all");
    bool hasNestedFile = std::any_of(lines.begin(), lines.end(), [&](const SourceLine& sl) {
      return sl.getPath().string().find("sentinel_workspace") != std::string::npos;
    });
    EXPECT_FALSE(hasNestedFile);
  }
}

}  // namespace sentinel
