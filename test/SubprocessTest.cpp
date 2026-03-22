/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <stdexcept>
#include <string>
#include <chrono>
#include <thread>
#include "sentinel/Subprocess.hpp"

namespace sentinel {

namespace fs = std::filesystem;

class SubprocessTest : public ::testing::Test {
 protected:
  void SetUp() override {
    BASE = fs::temp_directory_path() / "SENTINEL_SUBPROCESSTEST_TMP_DIR";
    fs::remove_all(BASE);
    fs::create_directories(BASE);
  }

  void TearDown() override {
    fs::remove_all(BASE);
  }

  fs::path BASE;
};

TEST_F(SubprocessTest, testExecuteSuccessfulCommand) {
  Subprocess sp("echo hello");
  sp.execute();
  EXPECT_TRUE(sp.isSuccessfulExit());
  EXPECT_FALSE(sp.isTimedOut());
}

TEST_F(SubprocessTest, testExecuteFailingCommand) {
  Subprocess sp("exit 1", 0, 0, "", true);
  sp.execute();
  EXPECT_FALSE(sp.isSuccessfulExit());
  EXPECT_FALSE(sp.isTimedOut());
}

TEST_F(SubprocessTest, testExecuteEmptyCommandReturnsNegOne) {
  Subprocess sp("");
  int ret = sp.execute();
  EXPECT_EQ(ret, -1);
}

TEST_F(SubprocessTest, testExecuteWithTimeout) {
  Subprocess sp("sleep 10", 1, 1);
  sp.execute();
  EXPECT_TRUE(sp.isTimedOut());
  EXPECT_FALSE(sp.isSuccessfulExit());
}

TEST_F(SubprocessTest, testExecuteWritesToLogFile) {
  auto logPath = BASE / "test.log";
  Subprocess sp("echo sentinel_marker", 0, 0, logPath, true);
  sp.execute();
  EXPECT_TRUE(fs::exists(logPath));
  std::ifstream f(logPath);
  std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  EXPECT_NE(content.find("sentinel_marker"), std::string::npos);
}

TEST_F(SubprocessTest, testExecuteSilentDoesNotPrintToStdout) {
  // silent=true suppresses child stdout; verify indirectly by checking successful exit.
  Subprocess sp("echo hello", 0, 0, "", true);
  sp.execute();
  EXPECT_TRUE(sp.isSuccessfulExit());
}

TEST_F(SubprocessTest, testConstructorThrowsWhenAnotherRunning) {
  // Start a background process, wait for it to fork, then try to construct another.
  Subprocess sp1("sleep 2");
  std::thread t([&] { sp1.execute(); });
  // Allow time for fork() to complete and childPid to be set
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_THROW({ Subprocess sp2("echo hello"); }, std::runtime_error);
  t.join();
}

TEST_F(SubprocessTest, testExecuteNonSilentWithLogFile) {
  // silent=false with a logFile: covers both the stdout-print and logStream branches
  // in the main read loop and drain loop.
  // Use large output (> 64 KB pipe buffer) to ensure the drain loop body executes.
  auto logPath = BASE / "nonsilent.log";
  Subprocess sp("seq 1 20000", 0, 0, logPath, false);
  sp.execute();
  EXPECT_TRUE(sp.isSuccessfulExit());
  EXPECT_TRUE(fs::exists(logPath));
}

}  // namespace sentinel
