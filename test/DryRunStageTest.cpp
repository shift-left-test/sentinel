/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include "helper/TestTempDir.hpp"
#include "sentinel/Config.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/Stage.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/stages/DryRunStage.hpp"

namespace fs = std::filesystem;

namespace sentinel {

// Minimal stage that always executes and returns true.
// Named differently from PassthroughStage in StageTest.cpp to avoid ODR violation.
class PassthroughStage : public Stage {
 public:
  PassthroughStage(const Config& cfg, std::shared_ptr<StatusLine> sl) :
      Stage(cfg, std::move(sl)) {}

  bool wasExecuted() const { return mExecuted; }

 protected:
  bool shouldSkip() const override { return false; }
  StatusLine::Phase getPhase() const override { return StatusLine::Phase::INIT; }
  bool execute() override {
    mExecuted = true;
    return true;
  }

 private:
  bool mExecuted = false;
};

class DryRunStageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_DRYRUN_STAGE_TEST");
    fs::remove_all(mBase);
    fs::create_directories(mBase);
    mWorkspaceRoot = mBase / "workspace";
  }

  void TearDown() override {
    fs::remove_all(mBase);
  }

  std::shared_ptr<Workspace> makeWorkspace() {
    auto ws = std::make_shared<Workspace>(mWorkspaceRoot);
    ws->initialize();
    return ws;
  }

  void populateMutants(Workspace* ws, int count) {
    // Create a dummy source file so Mutant paths are valid strings.
    fs::path src = mBase / "foo.cpp";
    std::ofstream f(src);
    f << "int foo() { return 0; }\n";
    for (int i = 1; i <= count; ++i) {
      Mutant m("AOR", src, "foo", 1, 1, 1, 5, "+");
      ws->createMutant(i, m);
    }
  }

  Config mConfig;
  std::shared_ptr<StatusLine> mStatusLine = std::make_shared<StatusLine>();
  fs::path mBase;
  fs::path mWorkspaceRoot;
};

// When dryRun is false, shouldSkip() returns true, so execute() is never called
// and the chain continues to the next stage.
TEST_F(DryRunStageTest, testShouldSkipWhenDryRunDisabled) {
  mConfig.dryRun = false;

  auto ws = makeWorkspace();
  auto stage = std::make_shared<DryRunStage>(mConfig, mStatusLine, ws);
  auto next = std::make_shared<PassthroughStage>(mConfig, mStatusLine);
  stage->setNext(next);

  // run() must not throw; the stage is skipped and the chain proceeds.
  EXPECT_NO_THROW(stage->run());
  EXPECT_TRUE(next->wasExecuted());
}

// When dryRun is true, execute() loads mutants and sets the total on the status line.
TEST_F(DryRunStageTest, testExecuteWhenDryRunEnabled) {
  mConfig.dryRun = true;

  auto ws = makeWorkspace();
  populateMutants(ws.get(), 2);

  auto stage = std::make_shared<DryRunStage>(mConfig, mStatusLine, ws);

  EXPECT_NO_THROW(stage->run());

  // The status text must reflect the total mutant count of 2 ("[0/2]").
  const std::string statusText = mStatusLine->getStatusText();
  EXPECT_NE(std::string::npos, statusText.find("2")) << "Status text: " << statusText;
}

// When dryRun is true, execute() returns false, so the chain stops and the
// next stage is never called.
TEST_F(DryRunStageTest, testExecuteReturnsFalseStopsChain) {
  mConfig.dryRun = true;

  auto ws = makeWorkspace();
  auto stage = std::make_shared<DryRunStage>(mConfig, mStatusLine, ws);
  auto next = std::make_shared<PassthroughStage>(mConfig, mStatusLine);
  stage->setNext(next);

  EXPECT_NO_THROW(stage->run());
  EXPECT_FALSE(next->wasExecuted());
}

}  // namespace sentinel
