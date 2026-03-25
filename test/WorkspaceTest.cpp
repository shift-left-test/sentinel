/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
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
#include "helper/TestTempDir.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class WorkspaceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_WORKSPACE_TEST");
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

  std::ifstream in(mRoot / "config.yaml");
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
  fs::create_directories(mRoot / "backup");  // already exists

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

TEST_F(WorkspaceTest, testMaxMutantCountFitsInFiveDigits) {
  // kMaxMutantCount must produce exactly 5 digits with the {:05d} format.
  // This documents the coupling between the constant and mutantDirName().
  std::string atLimit = fmt::format("{:05d}", Workspace::kMaxMutantCount);
  EXPECT_EQ(5u, atLimit.size());
}

TEST_F(WorkspaceTest, testAboveMaxMutantCountExceedsFiveDigits) {
  // One above the limit produces a 6-digit name, which would break the convention.
  std::string aboveLimit = fmt::format("{:05d}", Workspace::kMaxMutantCount + 1);
  EXPECT_EQ(6u, aboveLimit.size());
}

TEST_F(WorkspaceTest, testGetRoot) {
  Workspace ws(mRoot);
  EXPECT_EQ(mRoot, ws.getRoot());
}

TEST_F(WorkspaceTest, testGetActualDir) {
  Workspace ws(mRoot);
  EXPECT_EQ(mRoot / "actual", ws.getActualDir());
}

TEST_F(WorkspaceTest, testGetOriginalBuildLog) {
  Workspace ws(mRoot);
  EXPECT_EQ(mRoot / "original" / "build.log", ws.getOriginalBuildLog());
}

TEST_F(WorkspaceTest, testGetOriginalTestLog) {
  Workspace ws(mRoot);
  EXPECT_EQ(mRoot / "original" / "test.log", ws.getOriginalTestLog());
}

TEST_F(WorkspaceTest, testGetMutantBuildLog) {
  Workspace ws(mRoot);
  EXPECT_EQ(mRoot / "00001" / "build.log", ws.getMutantBuildLog(1));
}

TEST_F(WorkspaceTest, testGetMutantTestLog) {
  Workspace ws(mRoot);
  EXPECT_EQ(mRoot / "00001" / "test.log", ws.getMutantTestLog(1));
}

TEST_F(WorkspaceTest, testLoadMutantsSkipsNumericDirWithoutCfg) {
  Workspace ws(mRoot);
  ws.initialize();

  // Create a numeric directory without mt.cfg - should be skipped
  fs::create_directories(mRoot / "00002");

  Mutant m("AOR", mSrcFile, "func", 1, 1, 1, 5, "+");
  ws.createMutant(1, m);

  auto mutants = ws.loadMutants();
  ASSERT_EQ(1u, mutants.size());
  EXPECT_EQ(1, mutants[0].first);
}

TEST_F(WorkspaceTest, testGetDoneResultThrowsOnParseError) {
  Workspace ws(mRoot);
  ws.initialize();
  Mutant m("AOR", mSrcFile, "func", 1, 1, 1, 1, "+");
  ws.createMutant(1, m);
  // Write an empty mt.done file: valid open, but getline fails → parse error
  { std::ofstream f(mRoot / "00001" / "mt.done"); }
  EXPECT_THROW(ws.getDoneResult(1), std::runtime_error);
}

TEST_F(WorkspaceTest, testLoadStatusThrowsOnYamlTypeError) {
  Workspace ws(mRoot);
  ws.initialize();
  // Write an invalid value type for original-time to trigger YAML::TypeException
  std::ofstream f(mRoot / "status.yaml");
  f << "original-time: not_a_number\n";
  f.close();
  EXPECT_THROW(ws.loadStatus(), std::runtime_error);
}

TEST_F(WorkspaceTest, testSaveConfigFailsWhenPathIsDirectory) {
  Workspace ws(mRoot);
  ws.initialize();
  // Make config.yaml a directory so ofstream cannot open it
  fs::create_directories(mRoot / "config.yaml");
  EXPECT_THROW(ws.saveConfig("version: 1\n"), std::runtime_error);
}

TEST_F(WorkspaceTest, testSaveStatusFailsWhenRootMissing) {
  // mRoot does not exist (no initialize), so ofstream cannot write status.yaml
  Workspace ws(mRoot);
  WorkspaceStatus s;
  s.originalTime = 42;
  EXPECT_THROW(ws.saveStatus(s), std::runtime_error);
}

TEST_F(WorkspaceTest, testCreateMutantFailsWhenCfgIsDirectory) {
  Workspace ws(mRoot);
  ws.initialize();
  // Pre-create mt.cfg as a directory so ofstream fails
  fs::create_directories(mRoot / "00001" / "mt.cfg");
  Mutant m("AOR", mSrcFile, "func", 1, 1, 1, 1, "+");
  EXPECT_THROW(ws.createMutant(1, m), std::runtime_error);
}

TEST_F(WorkspaceTest, testSetLockFailsWhenMutantDirMissing) {
  Workspace ws(mRoot);
  ws.initialize();
  // Mutant dir 00001 does not exist → ofstream fails
  EXPECT_THROW(ws.setLock(1), std::runtime_error);
}

TEST_F(WorkspaceTest, testSetCompleteFailsWhenRootMissing) {
  // mRoot does not exist (no initialize) → ofstream fails
  Workspace ws(mRoot);
  EXPECT_THROW(ws.setComplete(), std::runtime_error);
}

TEST_F(WorkspaceTest, testSetDoneFailsWhenMutantDirMissing) {
  Workspace ws(mRoot);
  ws.initialize();
  Mutant m("AOR", mSrcFile, "func", 1, 1, 1, 1, "+");
  // Do not call createMutant, so mutant dir does not exist
  MutationResult result(m, "", "", MutationState::KILLED);
  EXPECT_THROW(ws.setDone(1, result), std::runtime_error);
}

TEST_F(WorkspaceTest, testIsCompleteReturnsFalseInitially) {
  Workspace ws(mRoot);
  ws.initialize();
  EXPECT_FALSE(ws.isComplete());
}

TEST_F(WorkspaceTest, testSetCompleteCreatesMarker) {
  Workspace ws(mRoot);
  ws.initialize();
  ws.setComplete();
  EXPECT_TRUE(ws.isComplete());
  EXPECT_TRUE(fs::exists(mRoot / "run.done"));
}

TEST_F(WorkspaceTest, testInitializeRemovesCompleteMarker) {
  Workspace ws(mRoot);
  ws.initialize();
  ws.setComplete();
  EXPECT_TRUE(ws.isComplete());
  ws.initialize();
  EXPECT_FALSE(ws.isComplete());
}

TEST_F(WorkspaceTest, testSaveAndLoadStatusOriginalTime) {
  Workspace ws(mRoot);
  ws.initialize();
  WorkspaceStatus s;
  s.originalTime = 42;
  ws.saveStatus(s);
  auto loaded = ws.loadStatus();
  ASSERT_TRUE(loaded.originalTime.has_value());
  EXPECT_EQ(*loaded.originalTime, 42u);
  EXPECT_FALSE(loaded.candidateCount.has_value());
}

TEST_F(WorkspaceTest, testSaveStatusReadModifyWrite) {
  Workspace ws(mRoot);
  ws.initialize();
  WorkspaceStatus a;
  a.originalTime = 10;
  ws.saveStatus(a);
  WorkspaceStatus b;
  b.candidateCount = 200;
  ws.saveStatus(b);
  auto loaded = ws.loadStatus();
  ASSERT_TRUE(loaded.originalTime.has_value());
  ASSERT_TRUE(loaded.candidateCount.has_value());
  EXPECT_EQ(*loaded.originalTime, 10u);
  EXPECT_EQ(*loaded.candidateCount, 200u);
}

TEST_F(WorkspaceTest, testLoadStatusReturnsEmptyWhenFileAbsent) {
  Workspace ws(mRoot);
  ws.initialize();
  auto loaded = ws.loadStatus();
  EXPECT_FALSE(loaded.originalTime.has_value());
  EXPECT_FALSE(loaded.candidateCount.has_value());
  EXPECT_FALSE(loaded.partIndex.has_value());
  EXPECT_FALSE(loaded.partCount.has_value());
}

TEST_F(WorkspaceTest, testSaveStatusAllFields) {
  Workspace ws(mRoot);
  ws.initialize();
  WorkspaceStatus s;
  s.originalTime = 5;
  s.candidateCount = 100;
  s.partIndex = 2;
  s.partCount = 4;
  ws.saveStatus(s);
  auto loaded = ws.loadStatus();
  ASSERT_TRUE(loaded.originalTime.has_value());
  ASSERT_TRUE(loaded.candidateCount.has_value());
  ASSERT_TRUE(loaded.partIndex.has_value());
  ASSERT_TRUE(loaded.partCount.has_value());
  EXPECT_EQ(*loaded.originalTime, 5u);
  EXPECT_EQ(*loaded.candidateCount, 100u);
  EXPECT_EQ(*loaded.partIndex, 2u);
  EXPECT_EQ(*loaded.partCount, 4u);
}

}  // namespace sentinel
