/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fstream>
#include <string>
#include "sentinel/Mutant.hpp"
#include "sentinel/SourceTree.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace sentinel {

namespace fs = std::filesystem;

SourceTree::SourceTree(const std::filesystem::path& baseDirectory) : mBaseDirectory(baseDirectory) {
  if (!fs::exists(mBaseDirectory)) {
    throw IOException(EINVAL);
  }
}

std::filesystem::path SourceTree::getBaseDirectory() const {
  return mBaseDirectory;
}

}  // namespace sentinel
