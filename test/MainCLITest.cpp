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
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include "helper/CaptureHelper.hpp"
#include "sentinel/MainCLI.hpp"
#include "sentinel/util/string.hpp"
#include "sentinel/Subprocess.hpp"

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
      R"a1f4(==============================================================
                   Mutant Population Report
==============================================================
File                                                 Mutants
--------------------------------------------------------------
sample.cpp                                                 3
--------------------------------------------------------------
TOTAL                                                      3
==============================================================)a1f4";

  std::string MUTATION_COVERAGE_REPORT2 =
      R"a1f4z0(==================================================================================
                             Mutation Coverage Report
==================================================================================
File                                                  Killed     Total     Score
----------------------------------------------------------------------------------
sample.cpp                                                 3         3    100.0%
----------------------------------------------------------------------------------
TOTAL                                                      3         3    100.0%
==================================================================================
)a1f4z0";

  // Sets up the minimum CLI args needed to reach the pre-run warning block
  // without running a real cmake/make project.  The run aborts early when the
  // user responds "n" to the warning prompt, so no build infrastructure is
  // needed.  The sentinel.yaml config is placed in SAMPLE_DIR and the cwd is
  // changed to SAMPLE_DIR so that no "working directory changed" warning fires.
  void setUpMinimalRunArgs() {
    namespace fs = std::experimental::filesystem;
    fs::current_path(SAMPLE_DIR);
    writeFile(SAMPLE_DIR / "sentinel.yaml", "{}\n");
    addArg("--config=sentinel.yaml");
    addArg(fmt::format("--source-dir={}", SAMPLE_DIR.string()).c_str());
    addArg(fmt::format("--compiledb-dir={}", SAMPLE_DIR.string()).c_str());
    addArg(fmt::format("--workspace={}", (SAMPLE_BASE / "workspace").string()).c_str());
    addArg(fmt::format("--test-report-dir={}", (SAMPLE_DIR / "testresult").string()).c_str());
    addArg("--build-command=make");
    addArg("--test-command=make test");
  }

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

  writeFile(SAMPLE_DIR / "sentinel.yaml", "{}\n");
  addArg(fmt::format("--config={}", (SAMPLE_DIR / "sentinel.yaml").string()).c_str());
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
  addArg("--force");

  captureStdout();
  sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();

  EXPECT_TRUE(fs::exists(SAMPLE_DIR / "result" / "mutations.xml"));
  EXPECT_TRUE(fs::exists(SAMPLE_DIR / "result" / "index.html"));
  EXPECT_TRUE(sentinel::string::contains(out, MUTATION_POPULATION_REPORT2));
  EXPECT_TRUE(sentinel::string::contains(out, MUTATION_COVERAGE_REPORT2));
}

TEST_F(MainCLITest, testThresholdPassWhenScoreAbove) {
  namespace fs = std::experimental::filesystem;

  makeGitRepo();

  Subprocess(fmt::format("cd {} && cmake .", SAMPLE_DIR.string())).execute();
  EXPECT_TRUE(fs::exists(SAMPLE_DIR / "compile_commands.json"));

  Subprocess(fmt::format("cd {} && make all", SAMPLE_DIR.string())).execute();
  Subprocess(fmt::format(R"a1b2(cd {} && GTEST_OUTPUT="xml:./testresult/" make test)a1b2", SAMPLE_DIR.string()))
      .execute();
  Subprocess(fmt::format("cd {} && make clean", SAMPLE_DIR.string())).execute();
  fs::remove_all(SAMPLE_DIR / "testresult");

  writeFile(SAMPLE_DIR / "sentinel.yaml", "{}\n");
  addArg(fmt::format("--config={}", (SAMPLE_DIR / "sentinel.yaml").string()).c_str());
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
  addArg("--force");
  addArg("--threshold=50");  // score will be 100%, well above threshold

  captureStdout();
  captureStderr();
  int ret = sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();
  auto err = capturedStderr();

  EXPECT_EQ(0, ret);
  EXPECT_TRUE(sentinel::string::contains(err, "Mutation score:"));
  EXPECT_TRUE(sentinel::string::contains(out, MUTATION_COVERAGE_REPORT2));
}

TEST_F(MainCLITest, testThresholdYamlConfigParsed) {
  // Verify that 'threshold' in sentinel.yaml is picked up and validated.
  setUpMinimalRunArgs();
  // Overwrite the sentinel.yaml written by setUpMinimalRunArgs() with a threshold value.
  writeFile(SAMPLE_DIR / "sentinel.yaml", "threshold: 101\n");

  captureStderr();
  int ret = sentinel::MainCLI(getArgc(), getArgv());
  auto err = capturedStderr();

  // sentinel.yaml has threshold: 101 which is out of range → exit 2
  EXPECT_EQ(2, ret);
  EXPECT_TRUE(sentinel::string::contains(err, "threshold"));
}

// ── --force option tests ───────────────────────────────────────────────────────

TEST_F(MainCLITest, testForceOptionSkipsInitOverwritePrompt) {
  // Pre-create sentinel.yaml with recognisable content.
  writeFile(SAMPLE_DIR / "sentinel.yaml", "# original\n");
  std::experimental::filesystem::current_path(SAMPLE_DIR);

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
  std::experimental::filesystem::current_path(SAMPLE_DIR);

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
  std::experimental::filesystem::current_path(SAMPLE_DIR);

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

// ── Pre-run configuration warning tests ──────────────────────────────────────

TEST_F(MainCLITest, testPreRunWarnLimitZeroAbortsOnNo) {
  setUpMinimalRunArgs();
  addArg("--limit=0");

  std::istringstream fakeInput("n\n");
  std::streambuf* origCin = std::cin.rdbuf(fakeInput.rdbuf());

  captureStdout();
  sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();
  std::cin.rdbuf(origCin);

  EXPECT_TRUE(sentinel::string::contains(out, "all candidate mutants will be evaluated"));
  EXPECT_TRUE(sentinel::string::contains(out, "Aborted."));
}

TEST_F(MainCLITest, testPreRunWarnTimeoutZeroAbortsOnNo) {
  setUpMinimalRunArgs();
  addArg("--timeout=0");

  std::istringstream fakeInput("n\n");
  std::streambuf* origCin = std::cin.rdbuf(fakeInput.rdbuf());

  captureStdout();
  sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();
  std::cin.rdbuf(origCin);

  EXPECT_TRUE(sentinel::string::contains(out, "no per-mutant test time limit"));
  EXPECT_TRUE(sentinel::string::contains(out, "Aborted."));
}

TEST_F(MainCLITest, testPreRunWarnExcludeTrailingSlashAbortsOnNo) {
  setUpMinimalRunArgs();
  addArg("--exclude=build/");

  std::istringstream fakeInput("n\n");
  std::streambuf* origCin = std::cin.rdbuf(fakeInput.rdbuf());

  captureStdout();
  sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();
  std::cin.rdbuf(origCin);

  EXPECT_TRUE(sentinel::string::contains(out, "ends with '/'"));
  EXPECT_TRUE(sentinel::string::contains(out, "Aborted."));
}

TEST_F(MainCLITest, testPreRunWarnExcludeNoWildcardAbortsOnNo) {
  setUpMinimalRunArgs();
  addArg("--exclude=build");

  std::istringstream fakeInput("n\n");
  std::streambuf* origCin = std::cin.rdbuf(fakeInput.rdbuf());

  captureStdout();
  sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();
  std::cin.rdbuf(origCin);

  EXPECT_TRUE(sentinel::string::contains(out, "relative pattern without a leading '*'"));
  EXPECT_TRUE(sentinel::string::contains(out, "Aborted."));
}

TEST_F(MainCLITest, testPreRunWarnPatternAbsoluteOutsideSrcAbortsOnNo) {
  setUpMinimalRunArgs();
  // Absolute path outside SAMPLE_DIR triggers the "outside source-dir" warning.
  addArg("--pattern=/tmp/other_repo/file.cpp");

  std::istringstream fakeInput("n\n");
  std::streambuf* origCin = std::cin.rdbuf(fakeInput.rdbuf());

  captureStdout();
  sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();
  std::cin.rdbuf(origCin);

  EXPECT_TRUE(sentinel::string::contains(out, "is an absolute path outside source-dir"));
  EXPECT_TRUE(sentinel::string::contains(out, "Aborted."));
}

TEST_F(MainCLITest, testPreRunWarnPatternAbsoluteInsideSrcAbortsOnNo) {
  namespace fs = std::experimental::filesystem;
  setUpMinimalRunArgs();
  // Absolute path inside SAMPLE_DIR triggers the "git pathspec" warning.
  addArg(fmt::format("--pattern={}", (SAMPLE_DIR / "file.cpp").string()).c_str());

  std::istringstream fakeInput("n\n");
  std::streambuf* origCin = std::cin.rdbuf(fakeInput.rdbuf());

  captureStdout();
  sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();
  std::cin.rdbuf(origCin);

  EXPECT_TRUE(sentinel::string::contains(out, "is an absolute path"));
  EXPECT_TRUE(sentinel::string::contains(out, "consider using a relative path"));
  EXPECT_TRUE(sentinel::string::contains(out, "Aborted."));
}

// ── --threshold option tests ───────────────────────────────────────────────────

TEST_F(MainCLITest, testThresholdInvalidHigh) {
  setUpMinimalRunArgs();
  addArg("--threshold=101");

  captureStderr();
  int ret = sentinel::MainCLI(getArgc(), getArgv());
  auto err = capturedStderr();

  // InvalidArgumentException is caught by MainCLI and returned as exit code 2
  EXPECT_EQ(2, ret);
  EXPECT_TRUE(sentinel::string::contains(err, "threshold"));
}

TEST_F(MainCLITest, testThresholdInvalidLow) {
  setUpMinimalRunArgs();
  addArg("--threshold=-1");

  captureStderr();
  int ret = sentinel::MainCLI(getArgc(), getArgv());
  auto err = capturedStderr();

  EXPECT_EQ(2, ret);
  EXPECT_TRUE(sentinel::string::contains(err, "threshold"));
}

TEST_F(MainCLITest, testPreRunWarningProceedsOnYes) {
  // With "y" the run proceeds past the warning block and must NOT print "Aborted.".
  // The build will fail (no real project), but that is acceptable here.
  setUpMinimalRunArgs();
  addArg("--timeout=0");

  std::istringstream fakeInput("y\n");
  std::streambuf* origCin = std::cin.rdbuf(fakeInput.rdbuf());

  captureStdout();
  try {
    sentinel::MainCLI(getArgc(), getArgv());
  } catch (...) {}
  auto out = capturedStdout();
  std::cin.rdbuf(origCin);

  EXPECT_FALSE(sentinel::string::contains(out, "Aborted."));
}

// ── --partition option tests ───────────────────────────────────────────────────

TEST_F(MainCLITest, testPartitionInvalidFormatNoSlash) {
  setUpMinimalRunArgs();
  addArg("--seed=42");
  addArg("--partition=2");

  captureStderr();
  int ret = sentinel::MainCLI(getArgc(), getArgv());
  auto err = capturedStderr();

  EXPECT_EQ(2, ret);
  EXPECT_TRUE(sentinel::string::contains(err, "partition"));
  EXPECT_TRUE(sentinel::string::contains(err, "N/TOTAL"));
}

TEST_F(MainCLITest, testPartitionInvalidFormatNonNumeric) {
  setUpMinimalRunArgs();
  addArg("--seed=42");
  addArg("--partition=a/5");

  captureStderr();
  int ret = sentinel::MainCLI(getArgc(), getArgv());
  auto err = capturedStderr();

  EXPECT_EQ(2, ret);
  EXPECT_TRUE(sentinel::string::contains(err, "partition"));
}

TEST_F(MainCLITest, testPartitionIndexZeroIsInvalid) {
  setUpMinimalRunArgs();
  addArg("--seed=42");
  addArg("--partition=0/5");

  captureStderr();
  int ret = sentinel::MainCLI(getArgc(), getArgv());
  auto err = capturedStderr();

  EXPECT_EQ(2, ret);
  EXPECT_TRUE(sentinel::string::contains(err, "partition"));
  EXPECT_TRUE(sentinel::string::contains(err, "N must be between"));
}

TEST_F(MainCLITest, testPartitionIndexExceedsCount) {
  setUpMinimalRunArgs();
  addArg("--seed=42");
  addArg("--partition=6/5");

  captureStderr();
  int ret = sentinel::MainCLI(getArgc(), getArgv());
  auto err = capturedStderr();

  EXPECT_EQ(2, ret);
  EXPECT_TRUE(sentinel::string::contains(err, "partition"));
  EXPECT_TRUE(sentinel::string::contains(err, "N must be between"));
}

TEST_F(MainCLITest, testPartitionCountZeroIsInvalid) {
  setUpMinimalRunArgs();
  addArg("--seed=42");
  addArg("--partition=1/0");

  captureStderr();
  int ret = sentinel::MainCLI(getArgc(), getArgv());
  auto err = capturedStderr();

  EXPECT_EQ(2, ret);
  EXPECT_TRUE(sentinel::string::contains(err, "partition"));
  EXPECT_TRUE(sentinel::string::contains(err, "TOTAL must be at least 1"));
}

TEST_F(MainCLITest, testPartitionWithoutSeedFails) {
  // --partition requires an explicit --seed; without it all partitions would
  // generate different random mutant lists.
  setUpMinimalRunArgs();
  addArg("--partition=2/5");
  // no --seed → defaults to "auto"

  captureStderr();
  int ret = sentinel::MainCLI(getArgc(), getArgv());
  auto err = capturedStderr();

  EXPECT_EQ(2, ret);
  EXPECT_TRUE(sentinel::string::contains(err, "partition"));
  EXPECT_TRUE(sentinel::string::contains(err, "seed"));
}

TEST_F(MainCLITest, testPartitionWithSeedAutoFails) {
  // Explicitly passing --seed=auto is the same as no seed.
  setUpMinimalRunArgs();
  addArg("--seed=auto");
  addArg("--partition=2/5");

  captureStderr();
  int ret = sentinel::MainCLI(getArgc(), getArgv());
  auto err = capturedStderr();

  EXPECT_EQ(2, ret);
  EXPECT_TRUE(sentinel::string::contains(err, "partition"));
  EXPECT_TRUE(sentinel::string::contains(err, "seed"));
}

TEST_F(MainCLITest, testPartitionDryRunShowsPartitionInfo) {
  namespace fs = std::experimental::filesystem;

  // Reset CWD: prior tests using setUpMinimalRunArgs() may have changed it.
  fs::current_path(fs::temp_directory_path());
  makeGitRepo();

  Subprocess(fmt::format("cd {} && cmake .", SAMPLE_DIR.string())).execute();
  EXPECT_TRUE(fs::exists(SAMPLE_DIR / "compile_commands.json"));

  Subprocess(fmt::format("cd {} && make all", SAMPLE_DIR.string())).execute();
  Subprocess(fmt::format(R"a1b2(cd {} && GTEST_OUTPUT="xml:./testresult/" make test)a1b2",
                         SAMPLE_DIR.string())).execute();
  Subprocess(fmt::format("cd {} && make clean", SAMPLE_DIR.string())).execute();
  fs::remove_all(SAMPLE_DIR / "testresult");

  writeFile(SAMPLE_DIR / "sentinel.yaml", "{}\n");
  addArg(fmt::format("--config={}", (SAMPLE_DIR / "sentinel.yaml").string()).c_str());
  addArg(fmt::format("--source-dir={}", SAMPLE_DIR.string()).c_str());
  addArg(fmt::format("--compiledb-dir={}", SAMPLE_DIR.string()).c_str());
  addArg(fmt::format("--workspace={}", (SAMPLE_BASE / "workspace").string()).c_str());
  addArg(fmt::format("--test-report-dir={}", (SAMPLE_DIR / "testresult").string()).c_str());
  addArg(fmt::format("--build-command={}", "make").c_str());
  addArg(fmt::format("--test-command={}", R"(GTEST_OUTPUT="xml:./testresult/" make test)").c_str());
  addArg("-sall");
  addArg("-l50");  // large enough to cover all sample mutants
  addArg("--timeout=auto");
  addArg("--seed=42");
  addArg("--generator=random");
  addArg(fmt::format("-e{}", SAMPLE_TEST_NAME).c_str());
  addArg("--force");
  addArg("--dry-run");
  addArg("--partition=1/3");

  captureStdout();
  int ret = sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();

  EXPECT_EQ(0, ret);
  EXPECT_TRUE(sentinel::string::contains(out, "partition:     1/3"));
  EXPECT_TRUE(sentinel::string::contains(out, "partition 1/3 of"));
}

TEST_F(MainCLITest, testPartitionDryRunTwoPartitionsMutantCountsSumToFull) {
  // Verify that running partition 1/2 and partition 2/2 in dry-run mode produces
  // mutant counts that sum to the full (non-partitioned) dry-run count.
  namespace fs = std::experimental::filesystem;

  // Reset CWD: prior tests using setUpMinimalRunArgs() may have changed it.
  fs::current_path(fs::temp_directory_path());
  makeGitRepo();

  Subprocess(fmt::format("cd {} && cmake .", SAMPLE_DIR.string())).execute();
  EXPECT_TRUE(fs::exists(SAMPLE_DIR / "compile_commands.json"));

  Subprocess(fmt::format("cd {} && make all", SAMPLE_DIR.string())).execute();
  Subprocess(fmt::format(R"a1b2(cd {} && GTEST_OUTPUT="xml:./testresult/" make test)a1b2",
                         SAMPLE_DIR.string())).execute();
  Subprocess(fmt::format("cd {} && make clean", SAMPLE_DIR.string())).execute();
  fs::remove_all(SAMPLE_DIR / "testresult");

  writeFile(SAMPLE_DIR / "sentinel.yaml", "{}\n");

  // Helper: build argv vector and run sentinel, returning stdout.
  auto runDryRun = [&](const std::string& workspace,
                       const std::string& partition) -> std::string {
    std::vector<std::string> args = {
      fmt::format("--config={}", (SAMPLE_DIR / "sentinel.yaml").string()),
      fmt::format("--source-dir={}", SAMPLE_DIR.string()),
      fmt::format("--compiledb-dir={}", SAMPLE_DIR.string()),
      fmt::format("--workspace={}", workspace),
      fmt::format("--test-report-dir={}", (SAMPLE_DIR / "testresult").string()),
      "--build-command=make",
      fmt::format("--test-command={}", R"(GTEST_OUTPUT="xml:./testresult/" make test)"),
      "-sall",
      "-l50",  // large enough to cover all sample mutants
      "--timeout=auto",
      "--seed=42",
      "--generator=random",
      fmt::format("-e{}", SAMPLE_TEST_NAME),
      "--force",
      "--dry-run",
    };
    if (!partition.empty()) {
      args.push_back(fmt::format("--partition={}", partition));
    }

    std::vector<char*> argv;
    argv.push_back(const_cast<char*>("./sentinel"));
    for (auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));  // cppcheck-suppress useStlAlgorithm

    captureStdout();
    sentinel::MainCLI(static_cast<int>(argv.size()), argv.data());
    return capturedStdout();
  };

  std::string fullOut = runDryRun((SAMPLE_BASE / "ws_full").string(), "");
  std::string p1Out   = runDryRun((SAMPLE_BASE / "ws_p1").string(), "1/2");
  std::string p2Out   = runDryRun((SAMPLE_BASE / "ws_p2").string(), "2/2");

  // Parse "Mutants: N " from each output line.
  auto parseMutantCount = [](const std::string& out) -> size_t {
    auto pos = out.find("Mutants: ");
    if (pos == std::string::npos) return 0;
    return static_cast<size_t>(std::stoul(out.substr(pos + 9)));
  };

  size_t fullN = parseMutantCount(fullOut);
  size_t p1N   = parseMutantCount(p1Out);
  size_t p2N   = parseMutantCount(p2Out);

  EXPECT_GT(fullN, 0u);
  EXPECT_EQ(fullN, p1N + p2N);

  // Partition outputs must show the partition tag.
  EXPECT_TRUE(sentinel::string::contains(p1Out, "partition 1/2 of"));
  EXPECT_TRUE(sentinel::string::contains(p2Out, "partition 2/2 of"));
}

}  // namespace sentinel
