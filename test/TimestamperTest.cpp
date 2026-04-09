/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <chrono>
#include <string>
#include <thread>
#include "sentinel/Timestamper.hpp"

namespace sentinel {

class TimestamperTest : public ::testing::Test {};

TEST_F(TimestamperTest, testInitialElapsedIsNearZero) {
  Timestamper ts;
  EXPECT_LT(ts.toDouble(), 0.1);
}

TEST_F(TimestamperTest, testElapsedGrowsOverTime) {
  Timestamper ts;
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_GE(ts.toDouble(), 0.15);
}

TEST_F(TimestamperTest, testResetRestartsClock) {
  Timestamper ts;
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  ts.reset();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  // After reset, only ~50ms elapsed, not ~250ms
  EXPECT_LT(ts.toDouble(), 0.15);
  EXPECT_GE(ts.toDouble(), 0.0);
}

TEST_F(TimestamperTest, testToStringClockFormatReturnsNonEmpty) {
  Timestamper ts;
  std::string s = ts.toString(Timestamper::Format::Clock);
  EXPECT_FALSE(s.empty());
}

TEST_F(TimestamperTest, testToStringHumanReadableFormatReturnsNonEmpty) {
  Timestamper ts;
  std::string s = ts.toString(Timestamper::Format::HumanReadable);
  EXPECT_FALSE(s.empty());
}

TEST_F(TimestamperTest, testFormatZeroSeconds) {
  EXPECT_EQ("0s", Timestamper::format(0.0));
}

TEST_F(TimestamperTest, testFormatSecondsOnly) {
  EXPECT_EQ("45s", Timestamper::format(45.3));
}

TEST_F(TimestamperTest, testFormatMinutesAndSeconds) {
  EXPECT_EQ("2m 30s", Timestamper::format(150.0));
}

TEST_F(TimestamperTest, testFormatHoursAndMinutes) {
  EXPECT_EQ("1h 1m", Timestamper::format(3661.0));
}

TEST_F(TimestamperTest, testFormatClockStyle) {
  EXPECT_EQ("01:02:03", Timestamper::format(3723.0, Timestamper::Format::Clock));
}

TEST_F(TimestamperTest, testToStringFormatsDiffer) {
  Timestamper ts;
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  std::string clock = ts.toString(Timestamper::Format::Clock);
  std::string human = ts.toString(Timestamper::Format::HumanReadable);
  EXPECT_NE(clock, human);
}

TEST_F(TimestamperTest, testFormatHoursOnly) {
  EXPECT_EQ("2h 0m", Timestamper::format(7200.0));
}

TEST_F(TimestamperTest, testFormatMinutesOnly) {
  EXPECT_EQ("1m 0s", Timestamper::format(60.0));
}

TEST_F(TimestamperTest, testFormatSubSecond) {
  EXPECT_EQ("0s", Timestamper::format(0.4));
}

TEST_F(TimestamperTest, testFormatClockZero) {
  EXPECT_EQ("00:00:00", Timestamper::format(0.0, Timestamper::Format::Clock));
}

TEST_F(TimestamperTest, testFormatClockWithHoursMinutesSeconds) {
  EXPECT_EQ("02:30:45", Timestamper::format(9045.0, Timestamper::Format::Clock));
}

TEST_F(TimestamperTest, testFormatUnknownFormatReturnsUnknown) {
  // Cast to create an invalid format value
  auto unknown = static_cast<Timestamper::Format>(99);
  EXPECT_EQ("Unknown format", Timestamper::format(10.0, unknown));
}

}  // namespace sentinel
