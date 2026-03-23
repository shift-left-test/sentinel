/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <stdexcept>
#include <memory>
#include <utility>
#include <vector>
#include "sentinel/Config.hpp"
#include "sentinel/Stage.hpp"
#include "sentinel/StatusLine.hpp"

namespace sentinel {

// Minimal concrete stage for testing
class RecordingStage : public Stage {
 public:
  RecordingStage(const Config& cfg, std::shared_ptr<StatusLine> sl, bool returnVal, bool skipVal = false) :
      Stage(cfg, std::move(sl)), mReturn(returnVal), mSkip(skipVal) {
  }
  bool wasExecuted() const { return mExecuted; }

 protected:
  bool shouldSkip() const override { return mSkip; }
  StatusLine::Phase getPhase() const override { return StatusLine::Phase::INIT; }
  bool execute() override {
    mExecuted = true;
    return mReturn;
  }

 private:
  bool mReturn;
  bool mSkip;
  bool mExecuted = false;
};

class ThrowingStage : public Stage {
 public:
  ThrowingStage(const Config& cfg, std::shared_ptr<StatusLine> sl) : Stage(cfg, std::move(sl)) {}

 protected:
  bool shouldSkip() const override { return false; }
  StatusLine::Phase getPhase() const override { return StatusLine::Phase::INIT; }
  bool execute() override { throw std::runtime_error("test error"); }
};

class StageTest : public ::testing::Test {
 protected:
  Config mConfig;
  std::shared_ptr<StatusLine> mStatusLine = std::make_shared<StatusLine>();
};

TEST_F(StageTest, testHandleCallsExecute) {
  auto stage = std::make_shared<RecordingStage>(mConfig, mStatusLine, true);
  stage->run();
  EXPECT_TRUE(stage->wasExecuted());
}

TEST_F(StageTest, testHandleProceedsToNextWhenExecuteReturnsTrue) {
  auto first = std::make_shared<RecordingStage>(mConfig, mStatusLine, true);
  auto second = std::make_shared<RecordingStage>(mConfig, mStatusLine, true);
  first->setNext(second);
  first->run();
  EXPECT_TRUE(first->wasExecuted());
  EXPECT_TRUE(second->wasExecuted());
}

TEST_F(StageTest, testHandleStopsWhenExecuteReturnsFalse) {
  auto first = std::make_shared<RecordingStage>(mConfig, mStatusLine, false);
  auto second = std::make_shared<RecordingStage>(mConfig, mStatusLine, true);
  first->setNext(second);
  first->run();
  EXPECT_TRUE(first->wasExecuted());
  EXPECT_FALSE(second->wasExecuted());
}

TEST_F(StageTest, testSetNextReturnsNextForChaining) {
  auto first = std::make_shared<RecordingStage>(mConfig, mStatusLine, true);
  auto second = std::make_shared<RecordingStage>(mConfig, mStatusLine, true);
  auto third = std::make_shared<RecordingStage>(mConfig, mStatusLine, true);
  first->setNext(second)->setNext(third);
  first->run();
  EXPECT_TRUE(first->wasExecuted());
  EXPECT_TRUE(second->wasExecuted());
  EXPECT_TRUE(third->wasExecuted());
}

TEST_F(StageTest, testSkippedStageDoesNotCallExecute) {
  auto stage = std::make_shared<RecordingStage>(mConfig, mStatusLine, true, /*skip=*/true);
  stage->run();
  EXPECT_FALSE(stage->wasExecuted());
}

TEST_F(StageTest, testSkippedStageProceedsToNext) {
  auto first = std::make_shared<RecordingStage>(mConfig, mStatusLine, true, /*skip=*/true);
  auto second = std::make_shared<RecordingStage>(mConfig, mStatusLine, true);
  first->setNext(second);
  first->run();
  EXPECT_FALSE(first->wasExecuted());
  EXPECT_TRUE(second->wasExecuted());
}

TEST_F(StageTest, testExceptionFromExecutePropagates) {
  auto stage = std::make_shared<ThrowingStage>(mConfig, mStatusLine);
  EXPECT_THROW(stage->run(), std::runtime_error);
}

}  // namespace sentinel
