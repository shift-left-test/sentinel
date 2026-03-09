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

SourceTree::SourceTree(const std::filesystem::path& baseDirectory) : mBaseDirectory(baseDirectory) {
  if (!std::filesystem::exists(mBaseDirectory)) {
    throw IOException(EINVAL);
  }
}

std::filesystem::path SourceTree::getBaseDirectory() const {
  return mBaseDirectory;
}

}  // namespace sentinel
