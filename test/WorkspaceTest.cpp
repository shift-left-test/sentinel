/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationState.hpp"
#include "sentinel/Workspace.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class WorkspaceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = fs::temp_directory_path() / "SENTINEL_WORKSPACE_TEST";
    fs::remove_all(mBase);
    fs::create_directories(mBase);
    mRoot = mBase / "workspace";
    // Create a real source file for use as Mutant path
    mSrcFile = mBase / "foo.cpp";
    std::ofstream f(mSrcFile);
    f << "int foo() { return 1; }\n";
  }

  void TearDown() override {
    fs::remove_all(mBase);
  }

  fs::path mBase;
  fs::path mRoot;
  fs::path mSrcFile;
};

TEST_F(WorkspaceTest, testHasPreviousRunReturnsFalseForNewWorkspace) {
  Workspace ws(mRoot);
  EXPECT_FALSE(ws.hasPreviousRun());
}

TEST_F(WorkspaceTest, testInitializeCreatesExpectedDirectories) {
  Workspace ws(mRoot);
  ws.initialize();

  EXPECT_TRUE(fs::is_directory(mRoot));
  EXPECT_TRUE(fs::is_directory(mRoot / "original"));
  EXPECT_TRUE(fs::is_directory(mRoot / "original" / "results"));
  EXPECT_TRUE(fs::is_directory(mRoot / "backup"));
}

TEST_F(WorkspaceTest, testSaveConfigCreatesFile) {
  Workspace ws(mRoot);
  ws.initialize();

  EXPECT_FALSE(ws.hasPreviousRun());
  ws.saveConfig("build-command: make\n");
  EXPECT_TRUE(ws.hasPreviousRun());

  std::ifstream in(mRoot / "sentinel.yaml");
  std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  EXPECT_EQ("build-command: make\n", content);
}

TEST_F(WorkspaceTest, testInitializeDeletesPreviousContents) {
  Workspace ws(mRoot);
  ws.initialize();
  ws.saveConfig("old-config: true\n");
  EXPECT_TRUE(ws.hasPreviousRun());

  ws.initialize();
  EXPECT_FALSE(ws.hasPreviousRun());
  EXPECT_TRUE(fs::is_directory(mRoot / "backup"));
}

TEST_F(WorkspaceTest, testGetOriginalDirAndResultsDir) {
  Workspace ws(mRoot);
  EXPECT_EQ(mRoot / "original", ws.getOriginalDir());
  EXPECT_EQ(mRoot / "original" / "results", ws.getOriginalResultsDir());
}

TEST_F(WorkspaceTest, testGetBackupDir) {
  Workspace ws(mRoot);
  EXPECT_EQ(mRoot / "backup", ws.getBackupDir());
}

TEST_F(WorkspaceTest, testCreateMutantCreatesDirectoryAndCfg) {
  Workspace ws(mRoot);
  ws.initialize();

  Mutant m("AOR", mSrcFile, "func", 10, 1, 10, 5, "+");
  ws.createMutant(1, m);

  EXPECT_TRUE(fs::is_directory(mRoot / "00001"));
  EXPECT_TRUE(fs::exists(mRoot / "00001" / "mt.cfg"));

  std::ifstream in(mRoot / "00001" / "mt.cfg");
  Mutant loaded;
  in >> loaded;
  EXPECT_EQ(m.getOperator(), loaded.getOperator());
  EXPECT_EQ(m.getFirst().line, loaded.getFirst().line);
  EXPECT_EQ(m.getToken(), loaded.getToken());
}

TEST_F(WorkspaceTest, testMutantDirNameIsFiveDigits) {
  Workspace ws(mRoot);
  ws.initialize();
  Mutant m("AOR", mSrcFile, "func", 1, 1, 1, 1, "+");
  ws.createMutant(42, m);
  EXPECT_TRUE(fs::is_directory(mRoot / "00042"));
}

TEST_F(WorkspaceTest, testLockLifecycle) {
  Workspace ws(mRoot);
  ws.initialize();
  Mutant m("AOR", mSrcFile, "func", 1, 1, 1, 1, "+");
  ws.createMutant(1, m);

  EXPECT_FALSE(ws.isLocked(1));
  ws.setLock(1);
  EXPECT_TRUE(ws.isLocked(1));
  EXPECT_TRUE(fs::exists(mRoot / "00001" / "mt.lock"));
  ws.clearLock(1);
  EXPECT_FALSE(ws.isLocked(1));
}

TEST_F(WorkspaceTest, testDoneLifecycle) {
  Workspace ws(mRoot);
  ws.initialize();
  Mutant m("AOR", mSrcFile, "func", 1, 1, 1, 1, "+");
  ws.createMutant(1, m);

  EXPECT_FALSE(ws.isDone(1));

  MutationResult result(m, "TestSuite.TestCase", "", MutationState::KILLED);
  ws.setDone(1, result);
  EXPECT_TRUE(ws.isDone(1));
  EXPECT_TRUE(fs::exists(mRoot / "00001" / "mt.done"));

  MutationResult loaded = ws.getDoneResult(1);
  EXPECT_EQ(MutationState::KILLED, loaded.getMutationState());
  EXPECT_EQ("TestSuite.TestCase", loaded.getKillingTest());
}

TEST_F(WorkspaceTest, testDonePreservesAllMutationStates) {
  Workspace ws(mRoot);
  ws.initialize();
  fs::path p = mSrcFile;

  const std::vector<MutationState> states = {MutationState::KILLED, MutationState::SURVIVED,
                                              MutationState::BUILD_FAILURE, MutationState::TIMEOUT};
  int id = 1;
  for (auto state : states) {
    Mutant m("AOR", p, "func", id, 1, id, 5, "+");
    ws.createMutant(id, m);
    MutationResult result(m, "", "", state);
    ws.setDone(id, result);

    MutationResult loaded = ws.getDoneResult(id);
    EXPECT_EQ(state, loaded.getMutationState());
    id++;
  }
}

TEST_F(WorkspaceTest, testLoadMutantsReturnsSortedById) {
  Workspace ws(mRoot);
  ws.initialize();
  fs::path p = mSrcFile;

  // Create mutants out of order
  Mutant m3("AOR", p, "func", 30, 1, 30, 5, "+");
  Mutant m1("BOR", p, "func", 10, 1, 10, 5, "|");
  Mutant m2("LCR", p, "func", 20, 1, 20, 5, "&&");
  ws.createMutant(3, m3);
  ws.createMutant(1, m1);
  ws.createMutant(2, m2);

  auto mutants = ws.loadMutants();
  ASSERT_EQ(3u, mutants.size());
  EXPECT_EQ(1, mutants[0].first);
  EXPECT_EQ(2, mutants[1].first);
  EXPECT_EQ(3, mutants[2].first);
  EXPECT_EQ("BOR", mutants[0].second.getOperator());
  EXPECT_EQ("LCR", mutants[1].second.getOperator());
  EXPECT_EQ("AOR", mutants[2].second.getOperator());
}

TEST_F(WorkspaceTest, testLoadMutantsIgnoresNonMutantDirectories) {
  Workspace ws(mRoot);
  ws.initialize();
  fs::path p = mSrcFile;

  Mutant m("AOR", p, "func", 1, 1, 1, 5, "+");
  ws.createMutant(1, m);

  // Non-mutant dirs should be ignored by loadMutants
  fs::create_directories(mRoot / "original");  // already exists
  fs::create_directories(mRoot / "backup");    // already exists

  auto mutants = ws.loadMutants();
  EXPECT_EQ(1u, mutants.size());
  EXPECT_EQ(1, mutants[0].first);
}

TEST_F(WorkspaceTest, testGetDoneResultThrowsIfMissingFile) {
  Workspace ws(mRoot);
  ws.initialize();
  Mutant m("AOR", mSrcFile, "func", 1, 1, 1, 1, "+");
  ws.createMutant(1, m);

  EXPECT_THROW(ws.getDoneResult(1), std::runtime_error);
}

}  // namespace sentinel
