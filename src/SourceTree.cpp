/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/SourceTree.hpp"


namespace sentinel {

SourceTree::SourceTree(
    const std::experimental::filesystem::path& baseDirectory) :
    mBaseDirectory(baseDirectory) {
  if (!std::experimental::filesystem::exists(mBaseDirectory)) {
    throw IOException(EINVAL);
  }
}

std::experimental::filesystem::path SourceTree::getBaseDirectory() const {
  return mBaseDirectory;
}

}  // namespace sentinel
