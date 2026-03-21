/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <memory>
#include <utility>
#include <vector>
#include "sentinel/Config.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/Stage.hpp"
#include "sentinel/StatusLine.hpp"

namespace sentinel {

// Minimal concrete stage for testing
class RecordingStage : public Stage {
 public:
  RecordingStage(const Config& cfg, StatusLine& sl, std::shared_ptr<Logger> log,
                 bool returnVal, int exitCode = 0)
      : Stage(cfg, sl, std::move(log)), mReturn(returnVal), mCode(exitCode) {}
  bool wasExecuted() const { return mExecuted; }
 protected:
  bool execute() override {
    mExecuted = true;
    setExitCode(mCode);
    return mReturn;
  }
 private:
  bool mReturn;
  int mCode;
  bool mExecuted = false;
};

class StageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    Logger::clearCache();
    mLogger = Logger::getLogger("test");
  }
  void TearDown() override { Logger::clearCache(); }
  Config mConfig;
  StatusLine mStatusLine;
  std::shared_ptr<Logger> mLogger;
};

TEST_F(StageTest, testHandleCallsExecute) {
  auto stage = std::make_shared<RecordingStage>(mConfig, mStatusLine, mLogger, true);
  stage->run();
  EXPECT_TRUE(stage->wasExecuted());
}

TEST_F(StageTest, testHandleProceedsToNextWhenExecuteReturnsTrue) {
  auto first = std::make_shared<RecordingStage>(mConfig, mStatusLine, mLogger, true);
  auto second = std::make_shared<RecordingStage>(mConfig, mStatusLine, mLogger, true);
  first->setNext(second);
  first->run();
  EXPECT_TRUE(first->wasExecuted());
  EXPECT_TRUE(second->wasExecuted());
}

TEST_F(StageTest, testHandleStopsWhenExecuteReturnsFalse) {
  auto first = std::make_shared<RecordingStage>(mConfig, mStatusLine, mLogger, false);
  auto second = std::make_shared<RecordingStage>(mConfig, mStatusLine, mLogger, true);
  first->setNext(second);
  first->run();
  EXPECT_TRUE(first->wasExecuted());
  EXPECT_FALSE(second->wasExecuted());
}

TEST_F(StageTest, testHandleReturnsExitCodeOnStop) {
  auto stage = std::make_shared<RecordingStage>(mConfig, mStatusLine, mLogger, false, 3);
  int code = stage->run();
  EXPECT_EQ(code, 3);
}

TEST_F(StageTest, testHandleReturnsZeroOnSuccess) {
  auto stage = std::make_shared<RecordingStage>(mConfig, mStatusLine, mLogger, true);
  int code = stage->run();
  EXPECT_EQ(code, 0);
}

TEST_F(StageTest, testSetNextReturnsNextForChaining) {
  auto first  = std::make_shared<RecordingStage>(mConfig, mStatusLine, mLogger, true);
  auto second = std::make_shared<RecordingStage>(mConfig, mStatusLine, mLogger, true);
  auto third  = std::make_shared<RecordingStage>(mConfig, mStatusLine, mLogger, true);
  first->setNext(second)->setNext(third);
  first->run();
  EXPECT_TRUE(first->wasExecuted());
  EXPECT_TRUE(second->wasExecuted());
  EXPECT_TRUE(third->wasExecuted());
}

TEST_F(StageTest, testExitCodePropagatesFromLastStage) {
  auto first = std::make_shared<RecordingStage>(mConfig, mStatusLine, mLogger, true);
  auto last = std::make_shared<RecordingStage>(mConfig, mStatusLine, mLogger, false, 42);
  first->setNext(last);
  int code = first->run();
  EXPECT_EQ(code, 42);
}

}  // namespace sentinel
