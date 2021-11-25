/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include "sentinel/MainCLI.hpp"


int main(int argc, char** argv) {
  if (argc > 1) {
    return sentinel::MainCLI(argc, argv);
  }

  if (argc == 1) {
    // TODO(loc.phan) : call main of GUI
    return sentinel::MainCLI(argc, argv);
  }

  return -1;
}
