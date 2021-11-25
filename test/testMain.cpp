/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include "sentinel/util/Subprocess.hpp"

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  sentinel::Subprocess checkCmake("cmake --help > /dev/null 2>&1");
  checkCmake.execute();
  if (!checkCmake.isSuccessfulExit()) {
    ::testing::GTEST_FLAG(filter) += ":-MainCLITest.testCommandRun";
  }

  return RUN_ALL_TESTS();
}
