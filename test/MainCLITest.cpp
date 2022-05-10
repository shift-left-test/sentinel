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
    git_repository *repo = nullptr;
    git_index* idx = nullptr;
    git_signature *me = nullptr;
    git_tree* tree = nullptr;

    git_libgit2_init();
    EXPECT_EQ(git_signature_now(&me, "Me", "me@example.com"), GIT_OK);
    EXPECT_EQ(git_repository_init(&repo, SAMPLE_DIR.c_str(), 0u), GIT_OK);
    EXPECT_EQ(git_repository_index(&idx, repo), GIT_OK);
    EXPECT_EQ(git_index_add_all(idx, nullptr, 0, nullptr, nullptr), GIT_OK);
    EXPECT_EQ(git_index_write_tree(&treeId, idx), GIT_OK);
    EXPECT_EQ(git_tree_lookup(&tree, repo, &treeId), GIT_OK);
    EXPECT_EQ(git_commit_create_v(&commitId, repo, "HEAD", me, me, nullptr,
        "Initial commit", tree, 0), GIT_OK);
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

  void writeFile(const std::experimental::filesystem::path& p,
      const std::string& c) {
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

  std::experimental::filesystem::path SAMPLE_COMCOMJSON_PATH;
  std::string SAMPLE_COMCOMJSON_NAME = "compile_commands.json";
  std::string SAMPLE_COMCOMJSON_CONTENTS =
      R"a1f4z([
{{
  "directory": "{0}",
  "command": "/usr/bin/c++      -o CMakeFiles/sample.dir/sample.cpp.o -c {0}/sample.cpp",
  "file": "{0}/sample.cpp"
}},
{{
  "directory": "{0}",
  "command": "/usr/bin/c++      -o CMakeFiles/sample_test.dir/sample.cpp.o -c {0}/sample.cpp",
  "file": "{0}/sample.cpp"
}},
{{
  "directory": "{0}",
  "command": "/usr/bin/c++      -o CMakeFiles/sample_test.dir/test_main.cpp.o -c {0}/test_main.cpp",
  "file": "{0}/test_main.cpp"
}}
]
)a1f4z";

  std::string MUTATION_POPULATION_REPORT1 =
      R"a1f4(--------------------------------------------------------------
                   Mutant Population Report
--------------------------------------------------------------
File                                               #mutation
--------------------------------------------------------------
sample.cpp                                                10
--------------------------------------------------------------
TOTAL                                                     10
--------------------------------------------------------------)a1f4";

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

  std::string MUTATION_COVERAGE_REPORT1 =
      R"a1f4(----------------------------------------------------------------------------------
                             Mutation Coverage Report
----------------------------------------------------------------------------------
File                                                 #killed #mutation       cov
----------------------------------------------------------------------------------
sample.cpp                                                 4         7       57%
----------------------------------------------------------------------------------
TOTAL                                                      4         7       57%
----------------------------------------------------------------------------------
Ignored Mutation
Build Failure                                                        0
Runtime Error                                                        0
Timeout                                                              3
----------------------------------------------------------------------------------)a1f4";

  std::string MUTATION_COVERAGE_REPORT2 =
     R"a1f4z0(----------------------------------------------------------------------------------
                             Mutation Coverage Report
----------------------------------------------------------------------------------
File                                                 #killed #mutation       cov
----------------------------------------------------------------------------------
sample.cpp                                                 1         2       50%
----------------------------------------------------------------------------------
TOTAL                                                      1         2       50%
----------------------------------------------------------------------------------
Ignored Mutation
Build Failure                                                        0
Runtime Error                                                        0
Timeout                                                              1
----------------------------------------------------------------------------------)a1f4z0";

  std::string EXPECTED_RESULT =
      R"a1d3(<?xml version="1.0" encoding="UTF-8"?>
<testsuites tests="1" failures="0" disabled="0" errors="0" timestamp="2021-01-21T02:48:54" time="0" name="AllTests">
  <testsuite name="SampleTest" tests="1" failures="0" disabled="0" errors="0" time="0">
    <testcase name="testSumOfEvenPositiveNumber" status="run" time="0" classname="SampleTest" />
  </testsuite>
</testsuites>
)a1d3";

  std::string ACTUAL_RESULT =
      R"ad13(<?xml version="1.0" encoding="UTF-8"?>
<testsuites tests="1" failures="1" disabled="0" errors="0" timestamp="2021-01-21T02:55:01" time="0" name="AllTests">
  <testsuite name="SampleTest" tests="1" failures="1" disabled="0" errors="0" time="0">
    <testcase name="testSumOfEvenPositiveNumber" status="run" time="0" classname="SampleTest">
      <failure message="{0}:7&#x0A;      Expected: sumOfEvenPositiveNumber(2, 10)&#x0A;      Which is: 0&#x0A;To be equal to: 24" type=""><![CDATA[{1}/test_main.cpp:7
      Expected: sumOfEvenPositiveNumber(2, 10)
      Which is: 0
To be equal to: 24]]></failure>
    </testcase>
  </testsuite>
</testsuites>
)ad13";

  std::string EVALUATION_RESULTS =
      R"azx2(		1			SOR,{0},sumOfEvenPositiveNumber,10,23,10,25,>>
SampleTest.testSumOfEvenPositiveNumber		0			ROR,{0},sumOfEvenPositiveNumber,10,17,10,19,<=
		4			UOI,{0},sumOfEvenPositiveNumber,9,29,9,31,((to)++)
		1			UOI,{0},sumOfEvenPositiveNumber,7,11,7,15,((from)++)
		4			UOI,{0},sumOfEvenPositiveNumber,9,26,9,27,((i)--)
SampleTest.testSumOfEvenPositiveNumber		0			LCR,{0},sumOfEvenPositiveNumber,10,9,10,37,1
		1			UOI,{0},sumOfEvenPositiveNumber,11,13,11,16,((ret)--)
SampleTest.testSumOfEvenPositiveNumber		0			SDL,{0},sumOfEvenPositiveNumber,11,7,11,21,{{}}
		4			UOI,{0},sumOfEvenPositiveNumber,10,10,10,11,((i)--)
SampleTest.testSumOfEvenPositiveNumber		0			ROR,{0},sumOfEvenPositiveNumber,10,32,10,37,0)azx2";

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
  EXPECT_TRUE(sentinel::string::contains(out,
      "./sentinel COMMAND {OPTIONS}"));
  EXPECT_TRUE(sentinel::string::contains(out,
      "populate                          Identify mutable test targets and"));
  EXPECT_TRUE(sentinel::string::contains(out,
      "-h, --help                        Display this help menu."));
}

TEST_F(MainCLITest, testCommandPopulate) {
  makeGitRepo();

  SAMPLE_COMCOMJSON_PATH = SAMPLE_DIR / SAMPLE_COMCOMJSON_NAME;
  writeFile(SAMPLE_COMCOMJSON_PATH,
      fmt::format(SAMPLE_COMCOMJSON_CONTENTS, SAMPLE_DIR.string()));

  addArg("populate");
  addArg(SAMPLE_DIR.c_str());
  addArg(fmt::format("-o{}", (SAMPLE_DIR / "work").string()).c_str());
  addArg(fmt::format("-b{}", SAMPLE_DIR.string()).c_str());
  addArg("-sall");
  addArg("-l10");
  addArg("--seed=0");
  addArg("--generator=random");
  addArg("--mutants-file-name=m.db");
  addArg(fmt::format("-e{}", SAMPLE_TEST_NAME).c_str());

  captureStdout();
  captureStderr();
  sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();
  EXPECT_TRUE(sentinel::string::contains(out, MUTATION_POPULATION_REPORT1));
  auto err = capturedStderr();
  EXPECT_EQ("", err);
  auto mdbContent = readFile(SAMPLE_DIR / "work" / "m.db");
  EXPECT_TRUE(sentinel::string::contains(mdbContent, fmt::format(
      R"a1f4(SOR,{},sumOfEvenPositiveNumber,10,23,10,25,>>)a1f4",
      SAMPLE_PATH.string())));
  EXPECT_TRUE(sentinel::string::contains(mdbContent, fmt::format(
      R"a1f4(UOI,{},sumOfEvenPositiveNumber,9,26,9,27,((i)--))a1f4",
      SAMPLE_PATH.string())));
  EXPECT_TRUE(sentinel::string::contains(mdbContent, fmt::format(
      R"a1f4(ROR,{},sumOfEvenPositiveNumber,10,32,10,37,0)a1f4",
      SAMPLE_PATH.string())));
}

TEST_F(MainCLITest, testCommandMutate) {
  addArg("mutate");
  addArg(SAMPLE_DIR.c_str());
  addArg(fmt::format("-w{}", (SAMPLE_BASE / "tmp").string()).c_str());
  addArg(fmt::format(
        R"a1d4(--mutant=SOR,{},sumOfEvenPositiveNumber,10,23,10,25,>>)a1d4",
        SAMPLE_PATH.string()).c_str());

  captureStdout();
  captureStderr();
  sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();
  EXPECT_EQ("", out);
  auto err = capturedStderr();
  EXPECT_EQ("", err);
  auto mutatedSample = readFile(SAMPLE_DIR / "sample.cpp");
  EXPECT_TRUE(sentinel::string::contains(mutatedSample,
        "if ((i & 1) == (1 >> 0) && i > 0) {"));
  EXPECT_TRUE(!sentinel::string::contains(mutatedSample,
        "if ((i & 1) == (1 << 0) && i > 0) {"));
  auto originalSample = readFile(SAMPLE_BASE/ "tmp" / "sample.cpp");
  EXPECT_TRUE(sentinel::string::contains(originalSample,
        "if ((i & 1) == (1 << 0) && i > 0) {"));
  EXPECT_TRUE(!sentinel::string::contains(originalSample,
        "if ((i & 1) == (1 >> 0) && i > 0) {"));
}

TEST_F(MainCLITest, testCommandEvaluate) {
  namespace fs = std::experimental::filesystem;
  auto EXPECTED_PATH = SAMPLE_BASE / "tmp" / "expect";
  auto ACTUAL_PATH = SAMPLE_BASE / "tmp" / "actual";
  fs::create_directories(EXPECTED_PATH);
  fs::create_directories(ACTUAL_PATH);
  writeFile(EXPECTED_PATH / "sample_test.xml", EXPECTED_RESULT);
  writeFile(ACTUAL_PATH / "sample_test.xml", fmt::format(ACTUAL_RESULT,
        SAMPLE_TEST_PATH.string(), SAMPLE_DIR.string()));

  addArg("evaluate");
  addArg(fmt::format("-o{}", (SAMPLE_BASE / "out").string()).c_str());
  addArg(fmt::format("--expected={}", EXPECTED_PATH.string()).c_str());
  addArg(fmt::format("--actual={}", ACTUAL_PATH.string()).c_str());
  addArg("--test-state=success");
  addArg(fmt::format(
        R"a1d4(--mutant=ROR,{},sumOfEvenPositiveNumber,10,32,10,37,0)a1d4",
        SAMPLE_PATH.string()).c_str());

  captureStdout();
  captureStderr();
  sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();
  EXPECT_TRUE(sentinel::string::contains(out,
        "sample.cpp (10:32-10:37 -> 0) KILLED"));
  auto err = capturedStderr();
  EXPECT_EQ("", err);
  auto evaluationResults = readFile(SAMPLE_BASE/ "out" / "EvaluationResults");
  EXPECT_TRUE(sentinel::string::contains(evaluationResults, fmt::format(
        R"k98d(SampleTest.testSumOfEvenPositiveNumber		0			ROR,{},sumOfEvenPositiveNumber,10,32,10,37,0)k98d",
        SAMPLE_PATH.string())));
}

TEST_F(MainCLITest, testCommandReport) {
  namespace fs = std::experimental::filesystem;
  writeFile(SAMPLE_BASE / "ev", fmt::format(EVALUATION_RESULTS,
        SAMPLE_PATH.string()));

  addArg("report");
  addArg(SAMPLE_DIR.c_str());
  addArg(fmt::format("-o{}", (SAMPLE_BASE / "out").string()).c_str());
  addArg(fmt::format("--evaluation-file={}",
        (SAMPLE_BASE / "ev").string()).c_str());

  captureStdout();
  captureStderr();
  sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();
  EXPECT_TRUE(sentinel::string::contains(out, MUTATION_COVERAGE_REPORT1));
  auto err = capturedStderr();
  EXPECT_EQ("", err);
  EXPECT_TRUE(fs::exists(SAMPLE_BASE / "out" / "mutations.xml"));
  EXPECT_TRUE(fs::exists(SAMPLE_BASE / "out" / "index.html"));
}

TEST_F(MainCLITest, testCommandRun) {
  namespace fs = std::experimental::filesystem;

  makeGitRepo();

  Subprocess(fmt::format("cd {} && cmake .", SAMPLE_DIR.string())).execute();
  EXPECT_TRUE(fs::exists(SAMPLE_DIR / "compile_commands.json"));

  // make timelimit
  Subprocess(fmt::format("cd {} && make all",
        SAMPLE_DIR.string())).execute();
  Subprocess(fmt::format(R"a1b2(cd {} && GTEST_OUTPUT="xml:./testresult/" make test)a1b2",
        SAMPLE_DIR.string())).execute();
  Subprocess(fmt::format("cd {} && make clean",
        SAMPLE_DIR.string())).execute();
  fs::remove_all(SAMPLE_DIR / "testresult");

  addArg("run");
  addArg(SAMPLE_DIR.c_str());
  addArg(fmt::format("-b{}", SAMPLE_DIR.string()).c_str());
  addArg(fmt::format("-o{}", (SAMPLE_DIR / "result").string()).c_str());
  addArg(fmt::format("--test-result-dir={}",
         (SAMPLE_DIR / "testresult").string()).c_str());
  addArg(fmt::format("--build-command={}", "make").c_str());
  addArg(fmt::format("--test-command={}",
        R"(GTEST_OUTPUT="xml:./testresult/" make test)").c_str());
  addArg("-sall");
  addArg("-l3");
  addArg("--timeout=auto");
  addArg("--seed=0");
  addArg("--generator=random");
  addArg(fmt::format("-e{}", SAMPLE_TEST_NAME).c_str());

  captureStdout();
  sentinel::MainCLI(getArgc(), getArgv());
  auto out = capturedStdout();
  EXPECT_TRUE(fs::exists(SAMPLE_DIR / "result" / "mutations.xml"));
  EXPECT_TRUE(fs::exists(SAMPLE_DIR / "result" / "index.html"));
  EXPECT_TRUE(sentinel::string::contains(out, MUTATION_POPULATION_REPORT2));
  EXPECT_TRUE(sentinel::string::contains(out,
        R"(SOR : sample.cpp (10:23-10:25 -> >>).............................. SURVIVED)"));
  EXPECT_TRUE(sentinel::string::contains(out,
      R"(ROR : sample.cpp (10:17-10:19 -> <=).............................. KILLED)"));
  EXPECT_TRUE(sentinel::string::contains(out,
      R"(UOI : sample.cpp (9:29-9:31 -> ((to)++)).......................... TIMEOUT)"));
  EXPECT_TRUE(sentinel::string::contains(out, MUTATION_COVERAGE_REPORT2));
}

}  // namespace sentinel
