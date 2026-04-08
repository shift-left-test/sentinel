/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "sentinel/Config.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/MutationState.hpp"
#include "sentinel/Workspace.hpp"
#include "helper/TestTempDir.hpp"
#include "helper/ThrowMessageMatcher.hpp"

namespace fs = std::filesystem;

using ::testing::AllOf;
using ::testing::HasSubstr;

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
  Config cfg = Config::withDefaults();
  cfg.buildCmd = "make";
  ws.saveConfig(cfg);
  EXPECT_TRUE(ws.hasPreviousRun());
  std::ifstream in(mRoot / "config.yaml");
  std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  EXPECT_NE(std::string::npos, content.find("build-command: make"));
}

TEST_F(WorkspaceTest, testInitializeDeletesPreviousContents) {
  Workspace ws(mRoot);
  ws.initialize();
  Config cfg = Config::withDefaults();
  ws.saveConfig(cfg);
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

TEST_F(WorkspaceTest, testRelativePathRoundTrip) {
  Workspace ws(mRoot);
  ws.initialize();

  fs::path relPath = fs::path("src") / "foo.cpp";
  Mutant m("AOR", relPath, "func", 10, 1, 10, 5, "+");
  ws.createMutant(1, m);

  // Verify mt.cfg round-trip preserves relative path
  auto mutants = ws.loadMutants();
  ASSERT_EQ(1u, mutants.size());
  EXPECT_EQ(relPath, mutants[0].second.getPath());
  EXPECT_TRUE(mutants[0].second.getPath().is_relative());

  // Verify mt.done round-trip preserves relative path
  MutationResult result(m, "TestCase", "", MutationState::KILLED);
  ws.setDone(1, result);
  MutationResult loaded = ws.getDoneResult(1);
  EXPECT_EQ(relPath, loaded.getMutant().getPath());
  EXPECT_TRUE(loaded.getMutant().getPath().is_relative());
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
  fs::create_directories(mRoot / "config.yaml");
  Config cfg = Config::withDefaults();
  EXPECT_THROW_MESSAGE(
      ws.saveConfig(cfg),
      std::runtime_error, AllOf(HasSubstr("config.yaml"), HasSubstr(":")));
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
  EXPECT_FALSE(loaded.version.has_value());
  EXPECT_FALSE(loaded.originalTime.has_value());
  EXPECT_FALSE(loaded.candidateCount.has_value());
  EXPECT_FALSE(loaded.partIndex.has_value());
  EXPECT_FALSE(loaded.partCount.has_value());
  EXPECT_FALSE(loaded.seed.has_value());
}

TEST_F(WorkspaceTest, testSaveStatusAllFields) {
  Workspace ws(mRoot);
  ws.initialize();
  WorkspaceStatus s;
  s.originalTime = 5;
  s.candidateCount = 100;
  s.partIndex = 2;
  s.partCount = 4;
  s.seed = 42u;
  ws.saveStatus(s);
  auto loaded = ws.loadStatus();
  ASSERT_TRUE(loaded.originalTime.has_value());
  ASSERT_TRUE(loaded.candidateCount.has_value());
  ASSERT_TRUE(loaded.partIndex.has_value());
  ASSERT_TRUE(loaded.partCount.has_value());
  ASSERT_TRUE(loaded.seed.has_value());
  EXPECT_EQ(*loaded.originalTime, 5u);
  EXPECT_EQ(*loaded.candidateCount, 100u);
  EXPECT_EQ(*loaded.partIndex, 2u);
  EXPECT_EQ(*loaded.partCount, 4u);
  EXPECT_EQ(*loaded.seed, 42u);
}

TEST(WorkspaceStatusTest, testStreamOperatorRoundTrip) {
  WorkspaceStatus original;
  original.originalTime = 42;
  original.candidateCount = 1000;
  original.partIndex = 2;
  original.partCount = 4;

  std::ostringstream out;
  out << original;
  std::istringstream in(out.str());
  WorkspaceStatus loaded;
  in >> loaded;

  ASSERT_TRUE(loaded.originalTime.has_value());
  ASSERT_TRUE(loaded.candidateCount.has_value());
  ASSERT_TRUE(loaded.partIndex.has_value());
  ASSERT_TRUE(loaded.partCount.has_value());
  EXPECT_EQ(*loaded.originalTime, 42u);
  EXPECT_EQ(*loaded.candidateCount, 1000u);
  EXPECT_EQ(*loaded.partIndex, 2u);
  EXPECT_EQ(*loaded.partCount, 4u);
}

TEST(WorkspaceStatusTest, testStreamOperatorPartialFields) {
  WorkspaceStatus original;
  original.originalTime = 10;

  std::ostringstream out;
  out << original;
  std::istringstream in(out.str());
  WorkspaceStatus loaded;
  in >> loaded;

  ASSERT_TRUE(loaded.originalTime.has_value());
  EXPECT_EQ(*loaded.originalTime, 10u);
  EXPECT_FALSE(loaded.candidateCount.has_value());
  EXPECT_FALSE(loaded.partIndex.has_value());
  EXPECT_FALSE(loaded.partCount.has_value());
}

TEST_F(WorkspaceTest, testGetMutantDirFiveDigitFormat) {
  Workspace ws(mRoot);
  EXPECT_EQ(mRoot / "00099", ws.getMutantDir(99));
  EXPECT_EQ(mRoot / "99999", ws.getMutantDir(99999));
}

TEST_F(WorkspaceTest, testSaveStatusPartialUpdatePreservesExistingFields) {
  Workspace ws(mRoot);
  ws.initialize();

  WorkspaceStatus a;
  a.originalTime = 30;
  a.candidateCount = 500;
  ws.saveStatus(a);

  WorkspaceStatus b;
  b.partIndex = 1;
  b.partCount = 3;
  ws.saveStatus(b);

  auto loaded = ws.loadStatus();
  ASSERT_TRUE(loaded.originalTime.has_value());
  ASSERT_TRUE(loaded.candidateCount.has_value());
  ASSERT_TRUE(loaded.partIndex.has_value());
  ASSERT_TRUE(loaded.partCount.has_value());
  EXPECT_EQ(30u, *loaded.originalTime);
  EXPECT_EQ(500u, *loaded.candidateCount);
  EXPECT_EQ(1u, *loaded.partIndex);
  EXPECT_EQ(3u, *loaded.partCount);
}

TEST_F(WorkspaceTest, testLoadMutantsReturnsEmptyForNoMutants) {
  Workspace ws(mRoot);
  ws.initialize();
  auto mutants = ws.loadMutants();
  EXPECT_TRUE(mutants.empty());
}

TEST_F(WorkspaceTest, testDonePreservesRuntimeErrorState) {
  Workspace ws(mRoot);
  ws.initialize();
  Mutant m("AOR", mSrcFile, "func", 1, 1, 1, 1, "+");
  ws.createMutant(1, m);

  MutationResult result(m, "", "crash info", MutationState::RUNTIME_ERROR);
  ws.setDone(1, result);

  MutationResult loaded = ws.getDoneResult(1);
  EXPECT_EQ(MutationState::RUNTIME_ERROR, loaded.getMutationState());
  EXPECT_EQ("crash info", loaded.getErrorTest());
}

TEST_F(WorkspaceTest, testMultipleMutantsLoadedInOrder) {
  Workspace ws(mRoot);
  ws.initialize();
  fs::path p = mSrcFile;

  const std::vector<int> createOrder = {5, 3, 1, 4, 2};
  for (int id : createOrder) {
    Mutant m("AOR", p, "func", id, 1, id, 1, "+");
    ws.createMutant(id, m);
  }

  auto mutants = ws.loadMutants();
  ASSERT_EQ(5u, mutants.size());
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(i + 1, mutants[i].first);
  }
}

TEST_F(WorkspaceTest, testSaveConfigPreservesAllFields) {
  Workspace ws(mRoot);
  ws.initialize();
  EXPECT_FALSE(ws.hasPreviousRun());

  Config cfg = Config::withDefaults();
  cfg.buildCmd = "cmake --build .";
  cfg.testCmd = "ctest -j4";
  cfg.sourceDir = mBase / "src";

  ws.saveConfig(cfg);
  EXPECT_TRUE(ws.hasPreviousRun());
  EXPECT_TRUE(fs::exists(mRoot / "config.yaml"));

  std::ifstream in(mRoot / "config.yaml");
  std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  EXPECT_NE(std::string::npos, content.find("build-command: cmake --build ."));
  EXPECT_NE(std::string::npos, content.find("test-command: ctest -j4"));
}

TEST_F(WorkspaceTest, testSaveAndLoadStatusMergedPartitions) {
  Workspace ws(mRoot);
  ws.initialize();
  WorkspaceStatus s;
  s.candidateCount = 5000;
  s.partCount = 3;
  s.mergedPartitions = std::vector<std::size_t>{1, 3};
  ws.saveStatus(s);
  auto loaded = ws.loadStatus();
  ASSERT_TRUE(loaded.mergedPartitions.has_value());
  EXPECT_EQ(*loaded.mergedPartitions, (std::vector<std::size_t>{1, 3}));
  ASSERT_TRUE(loaded.candidateCount.has_value());
  EXPECT_EQ(*loaded.candidateCount, 5000u);
}

TEST_F(WorkspaceTest, testLoadStatusMergedPartitionsAbsentByDefault) {
  Workspace ws(mRoot);
  ws.initialize();
  WorkspaceStatus s;
  s.originalTime = 10;
  ws.saveStatus(s);
  auto loaded = ws.loadStatus();
  EXPECT_FALSE(loaded.mergedPartitions.has_value());
}

TEST(WorkspaceStatusTest, testStreamOperatorMergedPartitionsRoundTrip) {
  WorkspaceStatus original;
  original.candidateCount = 5000;
  original.partCount = 3;
  original.mergedPartitions = std::vector<std::size_t>{1, 2, 3};

  std::ostringstream out;
  out << original;
  std::istringstream in(out.str());
  WorkspaceStatus loaded;
  in >> loaded;

  ASSERT_TRUE(loaded.mergedPartitions.has_value());
  EXPECT_EQ(*loaded.mergedPartitions, (std::vector<std::size_t>{1, 2, 3}));
}

TEST(WorkspaceStatusTest, testStreamOperatorSeedRoundTrip) {
  WorkspaceStatus original;
  original.seed = 12345u;
  original.candidateCount = 500;

  std::ostringstream out;
  out << original;
  std::istringstream in(out.str());
  WorkspaceStatus loaded;
  in >> loaded;

  ASSERT_TRUE(loaded.seed.has_value());
  EXPECT_EQ(*loaded.seed, 12345u);
  ASSERT_TRUE(loaded.candidateCount.has_value());
  EXPECT_EQ(*loaded.candidateCount, 500u);
}

TEST_F(WorkspaceTest, testLoadStatusSeedAbsentByDefault) {
  Workspace ws(mRoot);
  ws.initialize();
  WorkspaceStatus s;
  s.originalTime = 10;
  ws.saveStatus(s);
  auto loaded = ws.loadStatus();
  EXPECT_FALSE(loaded.seed.has_value());
}

TEST_F(WorkspaceTest, testSaveAndLoadStatusSeed) {
  Workspace ws(mRoot);
  ws.initialize();
  WorkspaceStatus s;
  s.seed = 42u;
  s.candidateCount = 100;
  ws.saveStatus(s);
  auto loaded = ws.loadStatus();
  ASSERT_TRUE(loaded.seed.has_value());
  EXPECT_EQ(*loaded.seed, 42u);
}

TEST_F(WorkspaceTest, testSaveStatusPreservesSeedDuringPartialUpdate) {
  Workspace ws(mRoot);
  ws.initialize();
  WorkspaceStatus a;
  a.seed = 99u;
  a.candidateCount = 500;
  ws.saveStatus(a);

  WorkspaceStatus b;
  b.partIndex = 1;
  b.partCount = 3;
  ws.saveStatus(b);

  auto loaded = ws.loadStatus();
  ASSERT_TRUE(loaded.seed.has_value());
  EXPECT_EQ(*loaded.seed, 99u);
  ASSERT_TRUE(loaded.partIndex.has_value());
  EXPECT_EQ(*loaded.partIndex, 1u);
}

TEST_F(WorkspaceTest, testSaveAndLoadStatusVersion) {
  Workspace ws(mRoot);
  ws.initialize();
  WorkspaceStatus s;
  s.version = "1.2.3";
  ws.saveStatus(s);
  auto loaded = ws.loadStatus();
  ASSERT_TRUE(loaded.version.has_value());
  EXPECT_EQ(*loaded.version, "1.2.3");
}

TEST_F(WorkspaceTest, testSaveStatusPreservesVersionDuringPartialUpdate) {
  Workspace ws(mRoot);
  ws.initialize();
  WorkspaceStatus a;
  a.version = "1.0.0";
  ws.saveStatus(a);

  WorkspaceStatus b;
  b.originalTime = 42;
  ws.saveStatus(b);

  auto loaded = ws.loadStatus();
  ASSERT_TRUE(loaded.version.has_value());
  EXPECT_EQ(*loaded.version, "1.0.0");
  ASSERT_TRUE(loaded.originalTime.has_value());
  EXPECT_EQ(*loaded.originalTime, 42u);
}

TEST(WorkspaceStatusTest, testStreamOperatorVersionRoundTrip) {
  WorkspaceStatus original;
  original.version = "0.4.8";
  original.candidateCount = 100;

  std::ostringstream out;
  out << original;
  std::istringstream in(out.str());
  WorkspaceStatus loaded;
  in >> loaded;

  ASSERT_TRUE(loaded.version.has_value());
  EXPECT_EQ(*loaded.version, "0.4.8");
  ASSERT_TRUE(loaded.candidateCount.has_value());
  EXPECT_EQ(*loaded.candidateCount, 100u);
}

TEST_F(WorkspaceTest, testHasMutantsReturnsFalseForEmptyWorkspace) {
  Workspace ws(mRoot);
  ws.initialize();
  EXPECT_FALSE(ws.hasMutants());
}

TEST_F(WorkspaceTest, testHasMutantsReturnsTrueWhenMutantExists) {
  Workspace ws(mRoot);
  ws.initialize();
  Mutant m("AOR", mSrcFile, "func", 1, 1, 1, 1, "+");
  ws.createMutant(1, m);
  EXPECT_TRUE(ws.hasMutants());
}

TEST_F(WorkspaceTest, testHasMutantsIgnoresNumericDirWithoutCfg) {
  Workspace ws(mRoot);
  ws.initialize();
  fs::create_directories(mRoot / "00001");
  EXPECT_FALSE(ws.hasMutants());
}

TEST_F(WorkspaceTest, testHasMutantsReturnsFalseForNonExistentRoot) {
  Workspace ws(mBase / "nonexistent");
  EXPECT_FALSE(ws.hasMutants());
}

TEST_F(WorkspaceTest, testLoadResultsReturnsEmptyForNoResults) {
  Workspace ws(mRoot);
  ws.initialize();
  Mutant m("AOR", mSrcFile, "func", 1, 1, 1, 1, "+");
  ws.createMutant(1, m);
  auto results = ws.loadResults();
  EXPECT_TRUE(results.empty());
}

TEST_F(WorkspaceTest, testLoadResultsReturnsDoneResults) {
  Workspace ws(mRoot);
  ws.initialize();
  fs::path p = mSrcFile;

  Mutant m1("AOR", p, "func", 1, 1, 1, 1, "+");
  Mutant m2("BOR", p, "func", 2, 1, 2, 1, "|");
  ws.createMutant(1, m1);
  ws.createMutant(2, m2);

  MutationResult r1(m1, "Test1", "", MutationState::KILLED);
  MutationResult r2(m2, "", "", MutationState::SURVIVED);
  ws.setDone(1, r1);
  ws.setDone(2, r2);

  auto results = ws.loadResults();
  ASSERT_EQ(2u, results.size());

  // Results may be in any order; check both are present
  bool foundKilled = false;
  bool foundSurvived = false;
  for (const auto& r : results) {
    if (r.getMutationState() == MutationState::KILLED) foundKilled = true;
    if (r.getMutationState() == MutationState::SURVIVED) foundSurvived = true;
  }
  EXPECT_TRUE(foundKilled);
  EXPECT_TRUE(foundSurvived);
}

TEST_F(WorkspaceTest, testLoadResultsIgnoresNonMutantDirectories) {
  Workspace ws(mRoot);
  ws.initialize();
  Mutant m("AOR", mSrcFile, "func", 1, 1, 1, 1, "+");
  ws.createMutant(1, m);
  MutationResult r(m, "Test1", "", MutationState::KILLED);
  ws.setDone(1, r);

  auto results = ws.loadResults();
  ASSERT_EQ(1u, results.size());
  EXPECT_EQ(MutationState::KILLED, results[0].getMutationState());
}

TEST(WorkspaceStatusTest, testVersionAppearsFirstInYaml) {
  WorkspaceStatus s;
  s.version = "0.4.8";
  s.originalTime = 10;
  s.candidateCount = 100;

  std::ostringstream out;
  out << s;
  std::string yaml = out.str();

  auto versionPos = yaml.find("version:");
  auto timePos = yaml.find("original-time:");
  ASSERT_NE(std::string::npos, versionPos);
  ASSERT_NE(std::string::npos, timePos);
  EXPECT_LT(versionPos, timePos);
}

}  // namespace sentinel
