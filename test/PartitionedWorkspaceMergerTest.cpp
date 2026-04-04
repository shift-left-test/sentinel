/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <string>
#include <vector>
#include "sentinel/Config.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationState.hpp"
#include "sentinel/PartitionedWorkspaceMerger.hpp"
#include "sentinel/Workspace.hpp"
#include "helper/TestTempDir.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class PartitionedWorkspaceMergerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_MERGER_TEST");
    fs::remove_all(mBase);
    fs::create_directories(mBase);
  }

  void TearDown() override {
    fs::remove_all(mBase);
  }

  fs::path createPartitionWorkspace(std::size_t partIndex,
                                    std::size_t partCount,
                                    std::size_t candidateCount = 100) {
    fs::path wsPath = mBase / fmt::format("part-{}", partIndex);
    Workspace ws(wsPath);
    ws.initialize();

    Config cfg = Config::withDefaults();
    cfg.buildCmd = "make";
    cfg.testCmd = "ctest";
    cfg.testResultDir = "/tmp/results";
    cfg.scope = "all";
    cfg.generator = "uniform";
    cfg.limit = 0;
    ws.saveConfig(cfg);

    WorkspaceStatus status;
    status.candidateCount = candidateCount;
    status.partIndex = partIndex;
    status.partCount = partCount;
    ws.saveStatus(status);

    fs::create_directories(wsPath / "original" / "results");
    {
      std::ofstream f(wsPath / "original" / "build.log");
      f << "build ok\n";
    }
    {
      std::ofstream f(wsPath / "original" / "test.log");
      f << "test ok\n";
    }

    ws.setComplete();
    return wsPath;
  }

  void addMutant(const fs::path& wsPath, int id,
                 MutationState state) {
    Workspace ws(wsPath);
    Mutant m("AOR", "src/foo.cpp", "func", id, 1, id, 5, "+");
    ws.createMutant(id, m);
    MutationResult result(m, "TestCase", "", state);
    ws.setDone(id, result);
    {
      std::ofstream f(ws.getMutantBuildLog(id));
      f << fmt::format("build mutant {}\n", id);
    }
    {
      std::ofstream f(ws.getMutantTestLog(id));
      f << fmt::format("test mutant {}\n", id);
    }
  }

  fs::path mBase;
};

// Source validation tests

TEST_F(PartitionedWorkspaceMergerTest,
       testThrowsWhenSourceMissingConfigYaml) {
  fs::path target = mBase / "merged";
  fs::path source = mBase / "bad-source";
  fs::create_directories(source);

  PartitionedWorkspaceMerger merger(target, {source}, false);
  EXPECT_THROW(merger.merge(), std::runtime_error);
}

TEST_F(PartitionedWorkspaceMergerTest,
       testThrowsWhenSourceMissingStatusYaml) {
  fs::path target = mBase / "merged";
  fs::path source = mBase / "bad-source";
  Workspace ws(source);
  ws.initialize();
  Config cfg = Config::withDefaults();
  cfg.buildCmd = "make";
  ws.saveConfig(cfg);

  PartitionedWorkspaceMerger merger(target, {source}, false);
  EXPECT_THROW(merger.merge(), std::runtime_error);
}

TEST_F(PartitionedWorkspaceMergerTest,
       testThrowsWhenSourceNotPartitionRun) {
  fs::path target = mBase / "merged";
  fs::path source = mBase / "non-partition";
  Workspace ws(source);
  ws.initialize();
  Config cfg = Config::withDefaults();
  cfg.buildCmd = "make";
  cfg.testCmd = "ctest";
  cfg.testResultDir = "/tmp/results";
  ws.saveConfig(cfg);

  WorkspaceStatus status;
  status.candidateCount = 100;
  status.partIndex = 0;
  status.partCount = 0;
  ws.saveStatus(status);
  ws.setComplete();

  PartitionedWorkspaceMerger merger(target, {source}, false);
  EXPECT_THROW(merger.merge(), std::runtime_error);
}

TEST_F(PartitionedWorkspaceMergerTest,
       testThrowsWhenSourceRunNotComplete) {
  fs::path target = mBase / "merged";
  fs::path source = mBase / "incomplete";
  Workspace ws(source);
  ws.initialize();
  Config cfg = Config::withDefaults();
  cfg.buildCmd = "make";
  cfg.testCmd = "ctest";
  cfg.testResultDir = "/tmp/results";
  ws.saveConfig(cfg);

  WorkspaceStatus status;
  status.candidateCount = 100;
  status.partIndex = 1;
  status.partCount = 3;
  ws.saveStatus(status);

  PartitionedWorkspaceMerger merger(target, {source}, false);
  EXPECT_THROW(merger.merge(), std::runtime_error);
}

// Compatibility validation tests

TEST_F(PartitionedWorkspaceMergerTest,
       testThrowsWhenPartCountMismatch) {
  fs::path target = mBase / "merged";
  fs::path src1 = createPartitionWorkspace(1, 3);
  fs::path src2dir = mBase / "part-1-alt";
  fs::create_directories(src2dir);
  fs::copy(mBase / "part-1", src2dir,
           fs::copy_options::recursive | fs::copy_options::overwrite_existing);
  {
    Workspace ws(src2dir);
    WorkspaceStatus s;
    s.partIndex = 2;
    s.partCount = 4;
    s.candidateCount = 100;
    ws.saveStatus(s);
  }

  PartitionedWorkspaceMerger merger(target, {src1, src2dir}, false);
  EXPECT_THROW(merger.merge(), std::runtime_error);
}

TEST_F(PartitionedWorkspaceMergerTest,
       testThrowsWhenDuplicatePartIndex) {
  fs::path target = mBase / "merged";
  fs::path src1 = createPartitionWorkspace(1, 3);
  fs::path src2dir = mBase / "part-1-dup";
  fs::copy(src1, src2dir, fs::copy_options::recursive);

  PartitionedWorkspaceMerger merger(target, {src1, src2dir}, false);
  EXPECT_THROW(merger.merge(), std::runtime_error);
}

TEST_F(PartitionedWorkspaceMergerTest,
       testThrowsWhenCandidateCountMismatch) {
  fs::path target = mBase / "merged";
  fs::path src1 = createPartitionWorkspace(1, 3, 100);
  fs::path src2 = createPartitionWorkspace(2, 3, 200);

  PartitionedWorkspaceMerger merger(target, {src1, src2}, false);
  EXPECT_THROW(merger.merge(), std::runtime_error);
}

TEST_F(PartitionedWorkspaceMergerTest,
       testThrowsWhenConfigMismatch) {
  fs::path target = mBase / "merged";
  fs::path src1 = createPartitionWorkspace(1, 3);
  fs::path src2 = createPartitionWorkspace(2, 3);

  {
    Config cfg = Config::withDefaults();
    cfg.buildCmd = "make";
    cfg.testCmd = "ctest";
    cfg.testResultDir = "/tmp/results";
    cfg.scope = "commit";
    Workspace ws(src2);
    ws.saveConfig(cfg);
  }

  PartitionedWorkspaceMerger merger(target, {src1, src2}, false);
  EXPECT_THROW(merger.merge(), std::runtime_error);
}

TEST_F(PartitionedWorkspaceMergerTest,
       testDifferentExcludedFieldsDoNotCauseConfigMismatch) {
  fs::path target = mBase / "merged";
  fs::path src1 = createPartitionWorkspace(1, 2);
  fs::path src2 = createPartitionWorkspace(2, 2);
  addMutant(src1, 1, MutationState::KILLED);
  addMutant(src2, 2, MutationState::SURVIVED);

  // Overwrite src2 config with different source-dir and output-dir
  {
    Config cfg = Config::withDefaults();
    cfg.buildCmd = "make";
    cfg.testCmd = "ctest";
    cfg.testResultDir = "/tmp/results";
    cfg.sourceDir = "/different/source/path";
    cfg.compileDbDir = "/different/compiledb/path";
    cfg.outputDir = "/different/output/path";
    Workspace ws(src2);
    ws.saveConfig(cfg);
  }

  PartitionedWorkspaceMerger merger(target, {src1, src2}, false);
  EXPECT_NO_THROW(merger.merge());
}

// Merge operation tests

TEST_F(PartitionedWorkspaceMergerTest,
       testMergesCopiesBaselineFromFirstSource) {
  fs::path target = mBase / "merged";
  fs::path src1 = createPartitionWorkspace(1, 2);
  addMutant(src1, 1, MutationState::KILLED);

  PartitionedWorkspaceMerger merger(target, {src1}, false);
  merger.merge();

  EXPECT_TRUE(fs::exists(target / "config.yaml"));
  EXPECT_TRUE(fs::exists(target / "original" / "build.log"));
  EXPECT_TRUE(fs::exists(target / "original" / "test.log"));
}

TEST_F(PartitionedWorkspaceMergerTest,
       testMergesCopiesMutantFiles) {
  fs::path target = mBase / "merged";
  fs::path src1 = createPartitionWorkspace(1, 2);
  fs::path src2 = createPartitionWorkspace(2, 2);
  addMutant(src1, 1, MutationState::KILLED);
  addMutant(src2, 2, MutationState::SURVIVED);

  PartitionedWorkspaceMerger merger(target, {src1, src2}, false);
  merger.merge();

  Workspace merged(target);
  EXPECT_TRUE(merged.isDone(1));
  EXPECT_TRUE(merged.isDone(2));
  EXPECT_TRUE(fs::exists(target / "00001" / "mt.cfg"));
  EXPECT_TRUE(fs::exists(target / "00001" / "mt.done"));
  EXPECT_TRUE(fs::exists(target / "00001" / "build.log"));
  EXPECT_TRUE(fs::exists(target / "00001" / "test.log"));
  EXPECT_TRUE(fs::exists(target / "00002" / "mt.cfg"));
  EXPECT_TRUE(fs::exists(target / "00002" / "mt.done"));
}

TEST_F(PartitionedWorkspaceMergerTest,
       testSkipsMutantWithoutMtDone) {
  fs::path target = mBase / "merged";
  fs::path src1 = createPartitionWorkspace(1, 2);

  Workspace ws(src1);
  Mutant m("AOR", "src/foo.cpp", "func", 1, 1, 1, 5, "+");
  ws.createMutant(1, m);

  PartitionedWorkspaceMerger merger(target, {src1}, false);
  merger.merge();

  EXPECT_FALSE(fs::exists(target / "00001"));
}

TEST_F(PartitionedWorkspaceMergerTest,
       testThrowsOnConflictingMutantResult) {
  fs::path target = mBase / "merged";
  fs::path src1 = createPartitionWorkspace(1, 2);
  addMutant(src1, 1, MutationState::KILLED);

  PartitionedWorkspaceMerger merger1(target, {src1}, false);
  merger1.merge();

  {
    std::ofstream f(src1 / "00001" / "mt.done");
    f << "different content\n";
  }

  PartitionedWorkspaceMerger merger2(target, {src1}, true);
  EXPECT_THROW(merger2.merge(), std::runtime_error);
}

TEST_F(PartitionedWorkspaceMergerTest,
       testForceAllowsIdenticalBaselineOverwrite) {
  fs::path target = mBase / "merged";
  fs::path src1 = createPartitionWorkspace(1, 3);
  fs::path src2 = createPartitionWorkspace(2, 3);
  addMutant(src1, 1, MutationState::KILLED);
  addMutant(src2, 2, MutationState::SURVIVED);

  // First merge: partition 1
  PartitionedWorkspaceMerger merger1(target, {src1}, false);
  merger1.merge();

  // Second merge: partition 2 with --force (baseline files already exist)
  PartitionedWorkspaceMerger merger2(target, {src2}, true);
  EXPECT_NO_THROW(merger2.merge());
}

TEST_F(PartitionedWorkspaceMergerTest,
       testIncrementalMergeSucceedsWhenBaselineAlreadyExists) {
  fs::path target = mBase / "merged";
  fs::path src1 = createPartitionWorkspace(1, 3);
  fs::path src2 = createPartitionWorkspace(2, 3);
  addMutant(src1, 1, MutationState::KILLED);
  addMutant(src2, 2, MutationState::SURVIVED);

  // First merge: partition 1 — copies baseline
  PartitionedWorkspaceMerger merger1(target, {src1}, false);
  merger1.merge();

  // Second merge: partition 2 — baseline already exists, skip copy, no error
  PartitionedWorkspaceMerger merger2(target, {src2}, false);
  EXPECT_NO_THROW(merger2.merge());
}

// Completeness tracking tests

TEST_F(PartitionedWorkspaceMergerTest,
       testAllPartitionsCollectedCreatesRunDone) {
  fs::path target = mBase / "merged";
  fs::path src1 = createPartitionWorkspace(1, 2);
  fs::path src2 = createPartitionWorkspace(2, 2);
  addMutant(src1, 1, MutationState::KILLED);
  addMutant(src2, 2, MutationState::SURVIVED);

  PartitionedWorkspaceMerger merger(target, {src1, src2}, false);
  merger.merge();

  Workspace merged(target);
  EXPECT_TRUE(merged.isComplete());
  auto status = merged.loadStatus();
  ASSERT_TRUE(status.mergedPartitions.has_value());
  EXPECT_EQ(*status.mergedPartitions,
            (std::vector<std::size_t>{1, 2}));
}

TEST_F(PartitionedWorkspaceMergerTest,
       testPartialMergeDoesNotCreateRunDone) {
  fs::path target = mBase / "merged";
  fs::path src1 = createPartitionWorkspace(1, 3);
  addMutant(src1, 1, MutationState::KILLED);

  PartitionedWorkspaceMerger merger(target, {src1}, false);
  merger.merge();

  Workspace merged(target);
  EXPECT_FALSE(merged.isComplete());
  auto status = merged.loadStatus();
  ASSERT_TRUE(status.mergedPartitions.has_value());
  EXPECT_EQ(*status.mergedPartitions,
            (std::vector<std::size_t>{1}));
  ASSERT_TRUE(status.partCount.has_value());
  EXPECT_EQ(*status.partCount, 3u);
}

TEST_F(PartitionedWorkspaceMergerTest, testIncrementalMerge) {
  fs::path target = mBase / "merged";
  fs::path src1 = createPartitionWorkspace(1, 3);
  fs::path src2 = createPartitionWorkspace(2, 3);
  fs::path src3 = createPartitionWorkspace(3, 3);
  addMutant(src1, 1, MutationState::KILLED);
  addMutant(src2, 2, MutationState::SURVIVED);
  addMutant(src3, 3, MutationState::KILLED);

  {
    PartitionedWorkspaceMerger merger(target, {src1}, false);
    merger.merge();
  }
  {
    Workspace merged(target);
    EXPECT_FALSE(merged.isComplete());
    auto status = merged.loadStatus();
    EXPECT_EQ(*status.mergedPartitions,
              (std::vector<std::size_t>{1}));
  }

  {
    PartitionedWorkspaceMerger merger(target, {src2, src3}, true);
    merger.merge();
  }
  {
    Workspace merged(target);
    EXPECT_TRUE(merged.isComplete());
    auto status = merged.loadStatus();
    EXPECT_EQ(*status.mergedPartitions,
              (std::vector<std::size_t>{1, 2, 3}));
  }
}

TEST_F(PartitionedWorkspaceMergerTest,
       testThrowsWhenDuplicateWithExistingMerge) {
  fs::path target = mBase / "merged";
  fs::path src1 = createPartitionWorkspace(1, 3);
  addMutant(src1, 1, MutationState::KILLED);

  {
    PartitionedWorkspaceMerger merger(target, {src1}, false);
    merger.merge();
  }

  fs::path src1dup = mBase / "part-1-dup2";
  fs::copy(src1, src1dup, fs::copy_options::recursive);
  {
    PartitionedWorkspaceMerger merger(target, {src1dup}, false);
    EXPECT_THROW(merger.merge(), std::runtime_error);
  }
}

}  // namespace sentinel
