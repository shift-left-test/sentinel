/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/GitSourceTree.hpp"
#include "sentinel/Logger.hpp"

namespace sentinel {
const char * cGitSourceTreeLoggerName = "GitSourceTree";

GitSourceTree::GitSourceTree(const std::string& baseDirectory) :
    SourceTree(baseDirectory) {
}

void GitSourceTree::modify(const Mutant& info, const std::experimental::filesystem::path& backupPath) {
  namespace fs = std::experimental::filesystem;

  // Backup target file to be mutated
  auto logger = Logger::getLogger(cGitSourceTreeLoggerName);

  fs::path targetFilename = fs::canonical(info.getPath());
  fs::path gitRootAbsolutePath = fs::canonical(getBaseDirectory());
  if (!string::startsWith(targetFilename.parent_path(), gitRootAbsolutePath)) {
    throw IOException(EINVAL, "Git root does not contain " + targetFilename.string());
  }
  std::string targetRelativePath = \
      targetFilename.parent_path().string().substr(gitRootAbsolutePath.string().length());
  fs::path newBackupPath = backupPath / targetRelativePath;
  if (!fs::exists(newBackupPath)) {
    fs::create_directories(newBackupPath);
  }
  fs::copy(targetFilename, newBackupPath, fs::copy_options::overwrite_existing);
  logger->info(fmt::format("backup: {}", newBackupPath.string()));

  // Apply mutation
  std::ifstream originalFile(targetFilename);
  std::stringstream buffer;
  if (originalFile) {
    buffer << originalFile.rdbuf();
    originalFile.close();
  } else {
    throw IOException(EINVAL, "Failed to open " + targetFilename.string());
  }
  std::ofstream mutatedFile(targetFilename, std::ios::trunc);

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
      mutatedFile << line << std::endl;
    }

    if (lineIdx == info.getFirst().line) {
      mutatedFile << line.substr(0, info.getFirst().column - 1);
      mutatedFile << info.getToken();
    }

    if (lineIdx == info.getLast().line) {
      mutatedFile << line.substr(info.getLast().column - 1) << std::endl;
    }
  }

  mutatedFile.close();
}

}  // namespace sentinel
