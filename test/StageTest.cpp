/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <stdexcept>
#include <filesystem>  // NOLINT
#include <memory>
#include <utility>
#include <vector>
#include "sentinel/PipelineContext.hpp"
#include "sentinel/Stage.hpp"
#include "sentinel/Workspace.hpp"
#include "helper/TestTempDir.hpp"

namespace sentinel {

namespace fs = std::filesystem;

class RecordingStage : public Stage {
 public:
  explicit RecordingStage(bool returnVal, bool skipVal = false) :
      mReturn(returnVal), mSkip(skipVal) {
  }
  bool wasExecuted() const { return mExecuted; }

 protected:
  bool shouldSkip(const PipelineContext& ctx) const override {
    (void)ctx;
    return mSkip;
  }
  StatusLine::Phase getPhase() const override { return StatusLine::Phase::INIT; }
  bool execute(PipelineContext* ctx) override {
    (void)ctx;
    mExecuted = true;
    return mReturn;
  }

 private:
  bool mReturn;
  bool mSkip;
  bool mExecuted = false;
};

class ThrowingStage : public Stage {
 protected:
  bool shouldSkip(const PipelineContext& ctx) const override {
    (void)ctx;
    return false;
  }
  StatusLine::Phase getPhase() const override { return StatusLine::Phase::INIT; }
  bool execute(PipelineContext* ctx) override {
    (void)ctx;
    throw std::runtime_error("test error");
  }
};

class StageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_STAGE_TEST");
    fs::remove_all(mBase);
    fs::create_directories(mBase);
    mWorkspace = std::make_shared<Workspace>(mBase / "ws");
  }
  void TearDown() override {
    fs::remove_all(mBase);
  }
  PipelineContext makeCtx() {
    return {mConfig, mStatusLine, *mWorkspace};
  }

  fs::path mBase;
  Config mConfig;
  StatusLine mStatusLine;
  std::shared_ptr<Workspace> mWorkspace;
};

TEST_F(StageTest, testHandleCallsExecute) {
  auto stage = std::make_shared<RecordingStage>(true);
  auto ctx = makeCtx();
  stage->run(&ctx);
  EXPECT_TRUE(stage->wasExecuted());
}

TEST_F(StageTest, testHandleProceedsToNextWhenExecuteReturnsTrue) {
  auto first = std::make_shared<RecordingStage>(true);
  auto second = std::make_shared<RecordingStage>(true);
  first->setNext(second);
  auto ctx = makeCtx();
  first->run(&ctx);
  EXPECT_TRUE(first->wasExecuted());
  EXPECT_TRUE(second->wasExecuted());
}

TEST_F(StageTest, testHandleStopsWhenExecuteReturnsFalse) {
  auto first = std::make_shared<RecordingStage>(false);
  auto second = std::make_shared<RecordingStage>(true);
  first->setNext(second);
  auto ctx = makeCtx();
  first->run(&ctx);
  EXPECT_TRUE(first->wasExecuted());
  EXPECT_FALSE(second->wasExecuted());
}

TEST_F(StageTest, testSetNextReturnsNextForChaining) {
  auto first = std::make_shared<RecordingStage>(true);
  auto second = std::make_shared<RecordingStage>(true);
  auto third = std::make_shared<RecordingStage>(true);
  first->setNext(second)->setNext(third);
  auto ctx = makeCtx();
  first->run(&ctx);
  EXPECT_TRUE(first->wasExecuted());
  EXPECT_TRUE(second->wasExecuted());
  EXPECT_TRUE(third->wasExecuted());
}

TEST_F(StageTest, testSkippedStageDoesNotCallExecute) {
  auto stage = std::make_shared<RecordingStage>(true, /*skip=*/true);
  auto ctx = makeCtx();
  stage->run(&ctx);
  EXPECT_FALSE(stage->wasExecuted());
}

TEST_F(StageTest, testSkippedStageProceedsToNext) {
  auto first = std::make_shared<RecordingStage>(true, /*skip=*/true);
  auto second = std::make_shared<RecordingStage>(true);
  first->setNext(second);
  auto ctx = makeCtx();
  first->run(&ctx);
  EXPECT_FALSE(first->wasExecuted());
  EXPECT_TRUE(second->wasExecuted());
}

TEST_F(StageTest, testExceptionFromExecutePropagates) {
  auto stage = std::make_shared<ThrowingStage>();
  auto ctx = makeCtx();
  EXPECT_THROW(stage->run(&ctx), std::runtime_error);
}

}  // namespace sentinel
