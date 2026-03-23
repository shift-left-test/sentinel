/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
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
  RecordingStage(const Config& cfg, std::shared_ptr<StatusLine> sl, bool returnVal) :
      Stage(cfg, std::move(sl)), mReturn(returnVal) {
  }
  bool wasExecuted() const {
    return mExecuted;
  }

 protected:
  bool execute() override {
    mExecuted = true;
    return mReturn;
  }

 private:
  bool mReturn;
  bool mExecuted = false;
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

}  // namespace sentinel
