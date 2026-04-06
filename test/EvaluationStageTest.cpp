/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include "git-harness/GitHarness.hpp"
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"
#include "sentinel/Config.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationState.hpp"
#include "sentinel/PipelineContext.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/stages/EvaluationStage.hpp"

namespace fs = std::filesystem;

using ::testing::HasSubstr;

namespace sentinel {

class EvaluationStageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_EVALSTAGE_TEST");
    fs::remove_all(mBase);
    fs::create_directories(mBase);
  }
  void TearDown() override {
    fs::remove_all(mBase);
  }
  fs::path mBase;
};

TEST_F(EvaluationStageTest, testRestoreBackupCopiesFilesToSrcRoot) {
  Workspace ws(mBase);
  auto srcRoot = mBase / "src";
  fs::create_directories(ws.getBackupDir());
  fs::create_directories(srcRoot);
  testutil::writeFile(ws.getBackupDir() / "foo.cpp", "original content");

  ws.restoreBackup(srcRoot);

  EXPECT_TRUE(fs::exists(srcRoot / "foo.cpp"));
  EXPECT_FALSE(fs::exists(ws.getBackupDir() / "foo.cpp"));
}

TEST_F(EvaluationStageTest, testRestoreBackupEmptyBackupIsNoop) {
  Workspace ws(mBase);
  auto srcRoot = mBase / "src";
  fs::create_directories(ws.getBackupDir());
  fs::create_directories(srcRoot);

  EXPECT_NO_THROW(ws.restoreBackup(srcRoot));
  EXPECT_TRUE(fs::is_empty(srcRoot));
}

TEST_F(EvaluationStageTest, testRestoreBackupNonexistentBackupIsNoop) {
  Workspace ws(mBase);
  auto srcRoot = mBase / "src";
  fs::create_directories(srcRoot);

  EXPECT_NO_THROW(ws.restoreBackup(srcRoot));
  EXPECT_TRUE(fs::is_empty(srcRoot));
}

TEST_F(EvaluationStageTest, testRestoreBackupPreservesSubdirectoryStructure) {
  Workspace ws(mBase);
  const auto srcRoot = mBase / "src";
  const auto deepBackup = ws.getBackupDir() / "sub" / "dir";
  fs::create_directories(deepBackup);
  fs::create_directories(srcRoot);
  testutil::writeFile(deepBackup / "deep.cpp", "deep content");

  ws.restoreBackup(srcRoot);

  EXPECT_TRUE(fs::exists(srcRoot / "sub" / "dir" / "deep.cpp"));
  EXPECT_FALSE(fs::exists(ws.getBackupDir() / "sub" / "dir" / "deep.cpp"));
}

TEST_F(EvaluationStageTest, testRestoreBackupOverwritesExisting) {
  Workspace ws(mBase);
  const auto srcRoot = mBase / "src";
  fs::create_directories(ws.getBackupDir());
  fs::create_directories(srcRoot);
  testutil::writeFile(srcRoot / "foo.cpp", "modified");
  testutil::writeFile(ws.getBackupDir() / "foo.cpp", "original");

  ws.restoreBackup(srcRoot);

  EXPECT_EQ("original", testutil::readFile(srcRoot / "foo.cpp"));
}

static const char* const kTestResultXml =
    "<?xml version=\"1.0\"?>\n"
    "<testsuites><testsuite name=\"S\" tests=\"1\">"
    "<testcase name=\"t1\" classname=\"C\" status=\"run\"/>"
    "</testsuite></testsuites>\n";

class EvaluationStageFlowTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_EVALFLOW_TEST");
    fs::remove_all(mBase);
    fs::create_directories(mBase);

    // Create a real git repo with a source file containing a mutation point.
    mRepoDir = mBase / "repo";
    mHarness = std::make_shared<GitHarness>(mRepoDir.string());
    mRepoDir = fs::canonical(mRepoDir);

    mHarness->addFile("foo.cpp", "int foo() { return 1 + 2; }\n");
    mHarness->stageFile({"foo.cpp"});
    mHarness->commit("init");

    // Set up workspace
    mWorkspaceRoot = mBase / "workspace";
    mWorkspace = std::make_shared<Workspace>(mWorkspaceRoot);
    mWorkspace->initialize();

    // Write original test results for the Evaluator
    auto origDir = mWorkspace->getOriginalResultsDir();
    fs::create_directories(origDir);
    testutil::writeFile(origDir / "results.xml", kTestResultXml);

    mTestResultDir = mBase / "test_results";
    fs::create_directories(mTestResultDir);

    mConfig.sourceDir = mRepoDir;
    mConfig.workDir = mWorkspaceRoot;
    mConfig.buildCmd = "true";
    mConfig.testCmd = "true";
    mConfig.testResultDir = mTestResultDir;
    mConfig.verbose = false;

    mGitRepo = std::make_shared<GitRepository>(mRepoDir);
  }

  void TearDown() override {
    fs::remove_all(mBase);
  }

  PipelineContext makeCtx() {
    return {mConfig, mStatusLine, *mWorkspace};
  }

  /**
   * @brief Create a single AOR mutant in the workspace.
   *
   * The mutant replaces '+' with '-' at line 1, columns 24-25 of foo.cpp.
   */
  void createDefaultMutant() {
    Mutant m("AOR", "foo.cpp", "foo", 1, 24, 1, 25, "-");
    mWorkspace->createMutant(1, m);
  }

  fs::path mBase;
  fs::path mRepoDir;
  fs::path mWorkspaceRoot;
  fs::path mTestResultDir;
  Config mConfig;
  StatusLine mStatusLine;
  std::shared_ptr<GitHarness> mHarness;
  std::shared_ptr<Workspace> mWorkspace;
  std::shared_ptr<GitRepository> mGitRepo;
};

TEST_F(EvaluationStageFlowTest, testShouldSkipWhenComplete) {
  createDefaultMutant();
  mWorkspace->setComplete();

  auto stage = std::make_shared<EvaluationStage>(mGitRepo);
  auto ctx = makeCtx();

  testing::internal::CaptureStdout();
  EXPECT_NO_THROW(stage->run(&ctx));
  std::string output = testing::internal::GetCapturedStdout();

  // When skipped, the stage should not produce evaluation output.
  EXPECT_THAT(output, ::testing::Not(HasSubstr("AOR")));
}

TEST_F(EvaluationStageFlowTest, testEvaluateMutantBuildFailure) {
  createDefaultMutant();
  mConfig.buildCmd = "false";

  auto stage = std::make_shared<EvaluationStage>(mGitRepo);
  auto ctx = makeCtx();

  testing::internal::CaptureStdout();
  EXPECT_NO_THROW(stage->run(&ctx));
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_THAT(output, HasSubstr("BUILD_FAILURE"));
  EXPECT_TRUE(mWorkspace->isComplete());
  EXPECT_TRUE(mWorkspace->isDone(1));

  auto result = mWorkspace->getDoneResult(1);
  EXPECT_EQ(MutationState::BUILD_FAILURE, result.getMutationState());
}

TEST_F(EvaluationStageFlowTest, testSkipAlreadyDoneMutant) {
  createDefaultMutant();

  // Mark mutant 1 as already done with a KILLED result.
  Mutant m("AOR", "foo.cpp", "foo", 1, 24, 1, 25, "-");
  MutationResult doneResult(m, "C.t1", "", MutationState::KILLED);
  mWorkspace->setDone(1, doneResult);

  auto stage = std::make_shared<EvaluationStage>(mGitRepo);
  auto ctx = makeCtx();

  testing::internal::CaptureStdout();
  EXPECT_NO_THROW(stage->run(&ctx));
  std::string output = testing::internal::GetCapturedStdout();

  // Already-done mutants are skipped (no output line for them).
  EXPECT_THAT(output, ::testing::Not(HasSubstr("AOR")));
  EXPECT_TRUE(mWorkspace->isComplete());
}

TEST_F(EvaluationStageFlowTest, testAutoTimeoutUsesWorkspaceStatus) {
  createDefaultMutant();

  // timeout=nullopt means auto; the stage reads originalTime from workspace status.
  mConfig.timeout = std::nullopt;

  WorkspaceStatus status;
  status.originalTime = 30;
  mWorkspace->saveStatus(status);

  auto stage = std::make_shared<EvaluationStage>(mGitRepo);
  auto ctx = makeCtx();

  testing::internal::CaptureStdout();
  EXPECT_NO_THROW(stage->run(&ctx));
  testing::internal::GetCapturedStdout();

  // The stage should complete without error, meaning auto timeout was resolved.
  EXPECT_TRUE(mWorkspace->isComplete());
  EXPECT_TRUE(mWorkspace->isDone(1));
}

TEST_F(EvaluationStageFlowTest, testExplicitTimeout) {
  createDefaultMutant();

  mConfig.timeout = 30;

  auto stage = std::make_shared<EvaluationStage>(mGitRepo);
  auto ctx = makeCtx();

  testing::internal::CaptureStdout();
  EXPECT_NO_THROW(stage->run(&ctx));
  testing::internal::GetCapturedStdout();

  EXPECT_TRUE(mWorkspace->isComplete());
  EXPECT_TRUE(mWorkspace->isDone(1));
}

}  // namespace sentinel
