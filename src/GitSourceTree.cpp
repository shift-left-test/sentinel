/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "sentinel/GitSourceTree.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

namespace fs = std::filesystem;

GitSourceTree::GitSourceTree(const std::filesystem::path& baseDirectory) : SourceTree(baseDirectory) {
}

void GitSourceTree::modify(const Mutant& info, const std::filesystem::path& backupPath) {
  // Backup target file to be mutated
  fs::path targetFilename = fs::canonical(info.getPath());
  fs::path gitRootAbsolutePath = fs::canonical(getBaseDirectory());
  if (!string::startsWith(targetFilename.parent_path().string(), gitRootAbsolutePath.string())) {
    throw IOException(EINVAL, "Git root does not contain " + targetFilename.string());
  }
  fs::path targetRelativePath = targetFilename.parent_path().lexically_relative(gitRootAbsolutePath);
  fs::path newBackupPath = backupPath / targetRelativePath;
  if (!fs::exists(newBackupPath)) {
    fs::create_directories(newBackupPath);
  }
  fs::copy(targetFilename, newBackupPath, fs::copy_options::overwrite_existing);

  // Apply mutation
  std::ifstream originalFile(targetFilename.string());
  std::stringstream buffer;
  if (originalFile) {
    buffer << originalFile.rdbuf();
    originalFile.close();
  } else {
    throw IOException(EINVAL, "Failed to open " + targetFilename.string());
  }
  std::ofstream mutatedFile(targetFilename.string(), std::ios::trunc);

  // If code line is out of target range, just write to mutant file.
  // If code line is in target range (start_line < code_line < end_line), skip.
  // If code line is on start_line, write the code appearing before start_col,
  // and write mutated token.
  // If code line is on end_line, write the code appearing after end_col.
  std::string line;
  std::size_t lineIdx = 0;

  while (std::getline(buffer, line)) {
    lineIdx += 1;

    if (lineIdx < info.getFirst().line || lineIdx > info.getLast().line) {
      mutatedFile << line << '\n';
    }

    if (lineIdx == info.getFirst().line) {
      mutatedFile << line.substr(0, info.getFirst().column - 1);
      mutatedFile << info.getToken();
    }

    if (lineIdx == info.getLast().line) {
      mutatedFile << line.substr(info.getLast().column - 1) << '\n';
    }
  }

  mutatedFile.close();
}

}  // namespace sentinel
