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
#include <string>
#include "sentinel/GitRepository.hpp"
#include "harness/git-harness/GitHarness.hpp"
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/util/filesystem.hpp"

namespace sentinel {

class GitRepositoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    repo_name = util::filesystem::tempPath("GitRepositoryTest-");
    repo = std::make_shared<GitHarness>(repo_name);
  }

  void TearDown() override {
    if (!HasFailure()) {
      util::filesystem::removeDirectories(repo_name);
    }
  }

  std::string repo_name;
  std::shared_ptr<GitHarness> repo;
};

TEST_F(GitRepositoryTest, testInvalidRepositoryThrow) {
    EXPECT_THROW({
        GitRepository gitRepo(".");

        SourceLines lines = gitRepo.getSourceLines();
    }, RepositoryException);
}

TEST_F(GitRepositoryTest, testGetSourceLinesWithNoCommit) {
  GitRepository gitRepo(repo_name);
  std::string content = "int main() {\n}\n";
  std::vector<std::string> stageFiles { "temp.cpp" };

  // nothing
  {
    SourceLines sourceLines = gitRepo.getSourceLines();

    EXPECT_TRUE(sourceLines.empty());
  }
  // only workdir
  {
    repo->addFile(stageFiles[0], content);
    SourceLines sourceLines = gitRepo.getSourceLines();

    EXPECT_TRUE(sourceLines.empty());
  }
  // only index
  {
    repo->stageFile(stageFiles);
        SourceLines sourceLines = gitRepo.getSourceLines();

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
    SourceLines sourceLines = gitRepo.getSourceLines();

    EXPECT_EQ(sourceLines.size(), 2);
  }
  // only workdir
  {
    repo->addCode(stageFiles[0], "int a = 1; a = a + 2;\n", 2, 1);
    SourceLines sourceLines = gitRepo.getSourceLines();

    EXPECT_EQ(sourceLines.size(), 3);
  }
  // only index
  {
    repo->stageFile(stageFiles);
    SourceLines sourceLines = gitRepo.getSourceLines();

    EXPECT_EQ(sourceLines.size(), 3);
  }
}

TEST_F(GitRepositoryTest, testGetSourceLines) {
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
    SourceLines sourceLines = gitRepo.getSourceLines();

    EXPECT_EQ(sourceLines.size(), 1);
    EXPECT_EQ(std::count(sourceLines.begin(), sourceLines.end(),
      SourceLine(stageFiles[0].c_str(), 2)), 1);
  }
  // only workdir
  {
    repo->addCode(stageFiles[0], "int b = 2; b = a + 4;\n", 3, 1);
    SourceLines sourceLines = gitRepo.getSourceLines();

    EXPECT_EQ(sourceLines.size(), 2);
    EXPECT_EQ(std::count(sourceLines.begin(), sourceLines.end(),
      SourceLine(stageFiles[0].c_str(), 3)), 1);
  }
  // only index
  {
    repo->stageFile(stageFiles);
    SourceLines sourceLines = gitRepo.getSourceLines();

    EXPECT_EQ(sourceLines.size(), 2);
    EXPECT_EQ(std::count(sourceLines.begin(), sourceLines.end(),
      SourceLine(stageFiles[0].c_str(), 3)), 1);
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
  SourceLines sourceLines = gitRepo.getSourceLines();

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

  SourceLines sourceLines = gitRepo.getSourceLines();
  EXPECT_EQ(sourceLines.size(), 4);
}

}  // namespace sentinel
