/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <experimental/filesystem>
#include <memory>
#include <string>
#include <vector>
#include "sentinel/GitRepository.hpp"
#include "harness/git-harness/GitHarness.hpp"
#include "helper/CaptureHelper.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace fs = std::experimental::filesystem;

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
  std::string tmpPath = repo_name / "test";
  fs::create_directories(tmpPath);

  EXPECT_THROW({
      GitRepository gitRepo(tmpPath);
      SourceLines lines = gitRepo.getSourceLines("commit");
    }, RepositoryException);
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
  std::vector<std::string> stageFiles { "temp.cpp" };

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
  std::vector<std::string> stageFiles { "temp.cpp" };

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
  std::vector<std::string> stageFiles { "temp.cpp" };
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
  std::vector<std::string> stageFiles { "temp.cpp" };

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
  std::vector<std::string> stageFiles { "temp.cpp" };

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
  std::vector<std::string> stageFiles { "temp.cpp" };

  repo->addFile(stageFiles[0], content);
  repo->stageFile(stageFiles);
  repo->commit("init");

  // branch work
  std::vector<std::string> targetBranches { "sub" };
  std::vector<std::string> subStageFiles { "test.cpp" };
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
  std::vector<std::string> stageFiles { "temp.cpp", "temp.c", "test/test.cpp" };

  repo->addFolder("test");

  for (auto &file : stageFiles) {
    repo->addFile(file, content);
  }
  GitRepository gitRepo(repo_name, {"cpp", "hpp"}, {}, {"*/test/*"});
  EXPECT_TRUE(gitRepo.isTargetPath("temp.cpp"));
  EXPECT_FALSE(gitRepo.isTargetPath("temp.c"));
  EXPECT_FALSE(gitRepo.isTargetPath("test/test.cpp"));
}

}  // namespace sentinel
