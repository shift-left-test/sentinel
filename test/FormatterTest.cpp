/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <string>
#include "sentinel/util/formatter.hpp"

namespace sentinel {

namespace fs = std::filesystem;

class FormatterTest : public ::testing::Test {};

TEST_F(FormatterTest, testRemovesDotDot) {
  fs::path p("/tmp/a/b/../file.cpp");
  EXPECT_EQ("/tmp/a/file.cpp", fmt::format("{}", p));
}

TEST_F(FormatterTest, testRemovesMultipleDotDots) {
  fs::path p("/tmp/a/b/c/../../file.cpp");
  EXPECT_EQ("/tmp/a/file.cpp", fmt::format("{}", p));
}

TEST_F(FormatterTest, testRemovesDot) {
  fs::path p("/tmp/a/./b/./file.cpp");
  EXPECT_EQ("/tmp/a/b/file.cpp", fmt::format("{}", p));
}

TEST_F(FormatterTest, testCleanAbsolutePathUnchanged) {
  fs::path p("/tmp/a/b/file.cpp");
  EXPECT_EQ("/tmp/a/b/file.cpp", fmt::format("{}", p));
}

TEST_F(FormatterTest, testRelativePathStaysRelative) {
  fs::path p("src/main.cpp");
  EXPECT_EQ("src/main.cpp", fmt::format("{}", p));
}

TEST_F(FormatterTest, testRelativePathWithDotDot) {
  fs::path p("src/../include/Console.hpp");
  EXPECT_EQ("include/Console.hpp", fmt::format("{}", p));
}

TEST_F(FormatterTest, testRemovesLeadingDotSlash) {
  fs::path p("./src/main.cpp");
  EXPECT_EQ("src/main.cpp", fmt::format("{}", p));
}

TEST_F(FormatterTest, testParentRelativePathPreserved) {
  fs::path p("../sentinel/src/main.cpp");
  EXPECT_EQ("../sentinel/src/main.cpp", fmt::format("{}", p));
}

TEST_F(FormatterTest, testRemovesDuplicateSlashes) {
  fs::path p("/tmp//a///b/file.cpp");
  EXPECT_EQ("/tmp/a/b/file.cpp", fmt::format("{}", p));
}

TEST_F(FormatterTest, testFilenameOnlyUnchanged) {
  fs::path p("file.cpp");
  EXPECT_EQ("file.cpp", fmt::format("{}", p));
}

}  // namespace sentinel
