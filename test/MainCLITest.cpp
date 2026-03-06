/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <git2.h>
#include <gtest/gtest.h>
#include <experimental/filesystem>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include "helper/CaptureHelper.hpp"
#include "sentinel/MainCLI.hpp"
#include "sentinel/util/string.hpp"
#include "sentinel/util/Subprocess.hpp"

namespace sentinel {

class MainCLITest : public ::testing::Test {
 protected:
  void SetUp() override {
    addArg("./sentinel");

    namespace fs = std::experimental::filesystem;
    SAMPLE_BASE = fs::temp_directory_path() / "SENTINEL_SAMPLE_DIR_MAIN_CLI";
    fs::remove_all(SAMPLE_BASE);
    SAMPLE_DIR = SAMPLE_BASE / "sample";
    fs::create_directories(SAMPLE_DIR);

    SAMPLE_PATH = SAMPLE_DIR / SAMPLE_NAME;
    writeFile(SAMPLE_PATH, SAMPLE_CONTENTS);

    SAMPLE_TEST_PATH = SAMPLE_DIR / SAMPLE_TEST_NAME;
    writeFile(SAMPLE_TEST_PATH, fmt::format(SAMPLE_TEST_CONTENTS, "TEST"));

    SAMPLE_CMAKELISTS_PATH = SAMPLE_DIR / SAMPLE_CMAKELISTS_NAME;
    writeFile(SAMPLE_CMAKELISTS_PATH, SAMPLE_CMAKELISTS_CONTENTS);

    mStdoutCapture = CaptureHelper::getStdoutCapture();
    mStderrCapture = CaptureHelper::getStderrCapture();
  }

  void TearDown() override {
    for (auto p : argVector) {
      delete[] p;
    }
    std::experimental::filesystem::remove_all(SAMPLE_BASE);
  }

  void makeGitRepo() {
    git_oid commitId, treeId;
    git_repository* repo = nullptr;
    git_index* idx = nullptr;
    git_signature* me = nullptr;
    git_tree* tree = nullptr;

    git_libgit2_init();
    EXPECT_EQ(git_signature_now(&me, "Me", "me@example.com"), GIT_OK);
    EXPECT_EQ(git_repository_init(&repo, SAMPLE_DIR.c_str(), 0u), GIT_OK);
    EXPECT_EQ(git_repository_index(&idx, repo), GIT_OK);
    EXPECT_EQ(git_index_add_all(idx, nullptr, 0, nullptr, nullptr), GIT_OK);
    EXPECT_EQ(git_index_write_tree(&treeId, idx), GIT_OK);
    EXPECT_EQ(git_tree_lookup(&tree, repo, &treeId), GIT_OK);
    EXPECT_EQ(git_commit_create_v(&commitId, repo, "HEAD", me, me, nullptr, "Initial commit", tree, 0), GIT_OK);
    EXPECT_EQ(git_index_write(idx), GIT_OK);

    git_signature_free(me);
    git_tree_free(tree);
    git_index_free(idx);
    git_repository_free(repo);
    git_libgit2_shutdown();
  }

  void addArg(const char* arg) {
    int len = std::strlen(arg) + 1;
    argVector.push_back(new char[len]);
    std::strncpy(argVector.back(), arg, len);
  }

  char** getArgv() {
    return argVector.data();
  }

  int getArgc() {
    return argVector.size();
  }

  void captureStdout() {
    mStdoutCapture->capture();
  }

  std::string capturedStdout() {
    return mStdoutCapture->release();
  }

  void captureStderr() {
    mStderrCapture->capture();
  }

  std::string capturedStderr() {
    return mStderrCapture->release();
  }

  void writeFile(const std::experimental::filesystem::path& p, const std::string& c) {
    std::ofstream t(p.string());
    t << c << std::endl;
    t.close();
  }

  std::string readFile(const std::experimental::filesystem::path& p) {
    EXPECT_TRUE(std::experimental::filesystem::exists(p));

    std::ifstream t(p);
    std::stringstream buffer;
    buffer << t.rdbuf();
    t.close();
    return buffer.str();
  }

  std::experimental::filesystem::path SAMPLE_BASE;
  std::experimental::filesystem::path SAMPLE_DIR;
  std::experimental::filesystem::path SAMPLE_PATH;
  std::string SAMPLE_NAME = "sample.cpp";
  std::string SAMPLE_CONTENTS =
      R"a1f4(inline bool lessThanOrEqual(int a, int b) {
  return a <= b;
}

int sumOfEvenPositiveNumber(int from, int to) {
  int ret = 0;
  int i = from;

  for (; lessThanOrEqual(i, to); ++i) {
    if ((i & 1) == (1 << 0) && i > 0) {
      ret = ret + i;
    }
  }
  return ret;
}
)a1f4";

  std::experimental::filesystem::path SAMPLE_TEST_PATH;
  std::string SAMPLE_TEST_NAME = "test_main.cpp";
  std::string SAMPLE_TEST_CONTENTS =
      R"a1f4(#include <gtest/gtest.h>


int sumOfEvenPositiveNumber(int from, int to);

{}(SampleTest, testSumOfEvenPositiveNumber) {{
    EXPECT_EQ(sumOfEvenPositiveNumber(2, 10), 24);
}}
)a1f4";

  std::experimental::filesystem::path SAMPLE_CMAKELISTS_PATH;
  std::string SAMPLE_CMAKELISTS_NAME = "CMakeLists.txt";
  std::string SAMPLE_CMAKELISTS_CONTENTS =
      R"a1f4(cmake_minimum_required(VERSION 3.3)

project(sample VERSION 0.0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(sample STATIC sample.cpp)

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

add_executable(sample_test sample.cpp test_main.cpp)
target_link_libraries(sample_test gtest gtest_main pthread)

add_test(
    NAME sample_test
    COMMAND sample_test
)
)a1f4";

  std::string MUTATION_POPULATION_REPORT2 =
      R"a1f4(--------------------------------------------------------------
                   Mutant Population Report
--------------------------------------------------------------
File                                               #mutation
--------------------------------------------------------------
sample.cpp                                                 3
--------------------------------------------------------------
TOTAL                                                      3
--------------------------------------------------------------)a1f4";

  std::string MUTATION_COVERAGE_REPORT2 =
      R"a1f4z0(----------------------------------------------------------------------------------
                             Mutation Coverage Report
----------------------------------------------------------------------------------
File                                                 #killed #mutation       cov
----------------------------------------------------------------------------------
sample.cpp                                                 3         3      100%
----------------------------------------------------------------------------------
TOTAL                                                      3         3      100%
----------------------------------------------------------------------------------)a1f4z0";

 private:
  std::vector<char*> argVector;
  std::shared_ptr<CaptureHelper> mStdoutCapture;
  std::shared_ptr<CaptureHelper> mStderrCapture;
};

TEST_F(MainCLITest, testHelpOption) {
  captureStdout();
  addArg("--help");
  sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();
  EXPECT_TRUE(sentinel::string::contains(out, "-h, --help                        Display this help menu."));
  EXPECT_TRUE(sentinel::string::contains(out, "--build-command"));
  EXPECT_TRUE(sentinel::string::contains(out, "--test-command"));
}

TEST_F(MainCLITest, testCommandRun) {
  namespace fs = std::experimental::filesystem;

  makeGitRepo();

  Subprocess(fmt::format("cd {} && cmake .", SAMPLE_DIR.string())).execute();
  EXPECT_TRUE(fs::exists(SAMPLE_DIR / "compile_commands.json"));

  // make timelimit
  Subprocess(fmt::format("cd {} && make all", SAMPLE_DIR.string())).execute();
  Subprocess(fmt::format(R"a1b2(cd {} && GTEST_OUTPUT="xml:./testresult/" make test)a1b2", SAMPLE_DIR.string()))
      .execute();
  Subprocess(fmt::format("cd {} && make clean", SAMPLE_DIR.string())).execute();
  fs::remove_all(SAMPLE_DIR / "testresult");

  addArg(fmt::format("--cwd={}", SAMPLE_DIR.string()).c_str());
  addArg(fmt::format("--source-dir={}", SAMPLE_DIR.string()).c_str());
  addArg(fmt::format("--compiledb-dir={}", SAMPLE_DIR.string()).c_str());
  addArg(fmt::format("-o{}", (SAMPLE_DIR / "result").string()).c_str());
  addArg(fmt::format("--workspace={}", (SAMPLE_BASE / "workspace").string()).c_str());
  addArg(fmt::format("--test-report-dir={}", (SAMPLE_DIR / "testresult").string()).c_str());
  addArg(fmt::format("--build-command={}", "make").c_str());
  addArg(fmt::format("--test-command={}", R"(GTEST_OUTPUT="xml:./testresult/" make test)").c_str());
  addArg("-sall");
  addArg("-l3");
  addArg("--timeout=auto");
  addArg("--seed=1");
  addArg("--generator=random");
  addArg(fmt::format("-e{}", SAMPLE_TEST_NAME).c_str());

  captureStdout();
  sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();

  EXPECT_TRUE(fs::exists(SAMPLE_DIR / "result" / "mutations.xml"));
  EXPECT_TRUE(fs::exists(SAMPLE_DIR / "result" / "index.html"));
  EXPECT_TRUE(sentinel::string::contains(out, MUTATION_POPULATION_REPORT2));
  EXPECT_TRUE(
      sentinel::string::contains(out, R"(ROR : sample.cpp (10:17-10:19 -> >)............................... KILLED)"));
  EXPECT_TRUE(
      sentinel::string::contains(out, R"(UOI : sample.cpp (9:29-9:31 -> (--(to))).......................... KILLED)"));
  EXPECT_TRUE(sentinel::string::contains(
      out, R"(UOI : sample.cpp (10:32-10:33 -> (++(i)))......................... KILLED)"));
  EXPECT_TRUE(sentinel::string::contains(out, MUTATION_COVERAGE_REPORT2));
}

// ── --force option tests ───────────────────────────────────────────────────────

TEST_F(MainCLITest, testForceOptionSkipsInitOverwritePrompt) {
  // Pre-create sentinel.yaml with recognisable content.
  writeFile(SAMPLE_DIR / "sentinel.yaml", "# original\n");

  addArg(fmt::format("--cwd={}", SAMPLE_DIR.string()).c_str());
  addArg("--init");
  addArg("--force");

  captureStdout();
  sentinel::MainCLI(getArgc(), getArgv());
  capturedStdout();  // discard

  // File must have been overwritten with the template (not the original stub).
  std::string content = readFile(SAMPLE_DIR / "sentinel.yaml");
  EXPECT_FALSE(sentinel::string::contains(content, "# original"));
  // Template always contains "build-command".
  EXPECT_TRUE(sentinel::string::contains(content, "build-command"));
}

TEST_F(MainCLITest, testInitPromptAbortsOnNo) {
  writeFile(SAMPLE_DIR / "sentinel.yaml", "# original\n");

  addArg(fmt::format("--cwd={}", SAMPLE_DIR.string()).c_str());
  addArg("--init");

  std::istringstream fakeInput("n\n");
  std::streambuf* origCin = std::cin.rdbuf(fakeInput.rdbuf());

  captureStdout();
  sentinel::MainCLI(getArgc(), getArgv());
  capturedStdout();  // discard
  std::cin.rdbuf(origCin);

  // File must NOT have been overwritten.
  std::string content = readFile(SAMPLE_DIR / "sentinel.yaml");
  EXPECT_TRUE(sentinel::string::contains(content, "# original"));
}

TEST_F(MainCLITest, testInitPromptOverwritesOnY) {
  writeFile(SAMPLE_DIR / "sentinel.yaml", "# original\n");

  addArg(fmt::format("--cwd={}", SAMPLE_DIR.string()).c_str());
  addArg("--init");

  std::istringstream fakeInput("y\n");
  std::streambuf* origCin = std::cin.rdbuf(fakeInput.rdbuf());

  captureStdout();
  sentinel::MainCLI(getArgc(), getArgv());
  capturedStdout();  // discard
  std::cin.rdbuf(origCin);

  // File must have been overwritten with the template.
  std::string content = readFile(SAMPLE_DIR / "sentinel.yaml");
  EXPECT_FALSE(sentinel::string::contains(content, "# original"));
  EXPECT_TRUE(sentinel::string::contains(content, "build-command"));
}

}  // namespace sentinel
