/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <memory>
#include <string>
#include "helper/TestTempDir.hpp"
#include "sentinel/Config.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationState.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/exceptions/ThresholdError.hpp"
#include "sentinel/stages/ReportStage.hpp"

namespace fs = std::filesystem;

namespace sentinel {

class ReportStageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_REPORTSTAGE_TEST");
    fs::remove_all(mBase);
    fs::create_directories(mBase);

    mSourceDir = mBase / "src";
    fs::create_directories(mSourceDir);

    // Create a real source file so MutationSummary can resolve it
    std::ofstream(mSourceDir / "foo.cpp") << "int foo(int a, int b) { return a + b; }\n";

    mWorkspaceDir = mBase / "workspace";
    mWorkspace = std::make_shared<Workspace>(mWorkspaceDir);
    mWorkspace->initialize();

    mStatusLine = std::make_shared<StatusLine>();
    Logger::setLevel(Logger::Level::OFF);
  }

  void TearDown() override {
    fs::remove_all(mBase);
    Logger::setLevel(Logger::Level::OFF);
  }

  /**
   * @brief Create a Mutant with a relative path (relative to mSourceDir).
   */
  Mutant makeMutant() const {
    return Mutant("AOR", fs::path("foo.cpp"), "foo", 1, 24, 1, 24, "-");
  }

  /**
   * @brief Register a mutant in the workspace and mark it done with the given state.
   */
  void addResult(int id, MutationState state) {
    Mutant m = makeMutant();
    mWorkspace->createMutant(id, m);
    MutationResult result(m, state == MutationState::KILLED ? "TestA" : "",
                          state == MutationState::RUNTIME_ERROR ? "TestA" : "", state);
    mWorkspace->setDone(id, result);
  }

  /**
   * @brief Build a Config with the test source directory.
   */
  Config makeConfig(const fs::path& outputDir = fs::path{},
                    std::optional<double> threshold = std::nullopt) const {
    Config cfg;
    cfg.sourceDir = mSourceDir;
    cfg.outputDir = outputDir;
    cfg.threshold = threshold;
    cfg.noTTY = true;
    return cfg;
  }

  fs::path mBase;
  fs::path mSourceDir;
  fs::path mWorkspaceDir;
  std::shared_ptr<Workspace> mWorkspace;
  std::shared_ptr<StatusLine> mStatusLine;
};

TEST_F(ReportStageTest, testShouldSkipAlwaysReturnsFalse) {
  Config cfg = makeConfig();
  ReportStage stage(cfg, mStatusLine, mWorkspace);
  EXPECT_NO_THROW(stage.run());
}

TEST_F(ReportStageTest, testNoMutantsNoThreshold) {
  Config cfg = makeConfig();
  Logger::setLevel(Logger::Level::INFO);

  testing::internal::CaptureStderr();
  ReportStage stage(cfg, mStatusLine, mWorkspace);
  EXPECT_NO_THROW(stage.run());
  std::string output = testing::internal::GetCapturedStderr();

  EXPECT_THAT(output, ::testing::HasSubstr("-"));
}

TEST_F(ReportStageTest, testAllKilledScoreIs100) {
  addResult(1, MutationState::KILLED);
  addResult(2, MutationState::KILLED);

  Config cfg = makeConfig();
  Logger::setLevel(Logger::Level::INFO);

  testing::internal::CaptureStderr();
  ReportStage stage(cfg, mStatusLine, mWorkspace);
  EXPECT_NO_THROW(stage.run());
  std::string output = testing::internal::GetCapturedStderr();

  EXPECT_THAT(output, ::testing::HasSubstr("100.0%"));
}

TEST_F(ReportStageTest, testMixedResultsScore) {
  addResult(1, MutationState::KILLED);
  addResult(2, MutationState::KILLED);
  addResult(3, MutationState::SURVIVED);
  addResult(4, MutationState::SURVIVED);

  Config cfg = makeConfig();
  Logger::setLevel(Logger::Level::INFO);

  testing::internal::CaptureStderr();
  ReportStage stage(cfg, mStatusLine, mWorkspace);
  EXPECT_NO_THROW(stage.run());
  std::string output = testing::internal::GetCapturedStderr();

  EXPECT_THAT(output, ::testing::HasSubstr("50.0%"));
}

TEST_F(ReportStageTest, testThresholdPassedDoesNotThrow) {
  // 2 killed, 1 survived => score = 66.7%, threshold = 50.0
  addResult(1, MutationState::KILLED);
  addResult(2, MutationState::KILLED);
  addResult(3, MutationState::SURVIVED);

  Config cfg = makeConfig(fs::path{}, 50.0);
  ReportStage stage(cfg, mStatusLine, mWorkspace);
  EXPECT_NO_THROW(stage.run());
}

TEST_F(ReportStageTest, testThresholdFailedThrowsThresholdError) {
  // 1 killed, 3 survived => score = 25.0%, threshold = 50.0
  addResult(1, MutationState::KILLED);
  addResult(2, MutationState::SURVIVED);
  addResult(3, MutationState::SURVIVED);
  addResult(4, MutationState::SURVIVED);

  Config cfg = makeConfig(fs::path{}, 50.0);
  ReportStage stage(cfg, mStatusLine, mWorkspace);
  EXPECT_THROW(stage.run(), ThresholdError);
}

TEST_F(ReportStageTest, testThresholdWithNoMutationsDoesNotThrow) {
  // No mutants => score is nullopt; threshold should not trigger
  Config cfg = makeConfig(fs::path{}, 80.0);
  ReportStage stage(cfg, mStatusLine, mWorkspace);
  EXPECT_NO_THROW(stage.run());
}

TEST_F(ReportStageTest, testOutputDirSavesReports) {
  addResult(1, MutationState::KILLED);

  fs::path outputDir = mBase / "reports";
  Config cfg = makeConfig(outputDir);
  ReportStage stage(cfg, mStatusLine, mWorkspace);
  EXPECT_NO_THROW(stage.run());

  EXPECT_TRUE(fs::exists(outputDir));
  EXPECT_TRUE(fs::exists(outputDir / "mutations.xml"));
}

TEST_F(ReportStageTest, testEmptyOutputDirDoesNotSaveReports) {
  addResult(1, MutationState::KILLED);

  Config cfg = makeConfig(fs::path{});
  ReportStage stage(cfg, mStatusLine, mWorkspace);
  EXPECT_NO_THROW(stage.run());

  // No report directory should have been created under mBase beyond src/ and workspace/
  EXPECT_FALSE(fs::exists(mBase / "reports"));
  EXPECT_FALSE(fs::exists(mBase / "mutations.xml"));
}

}  // namespace sentinel
