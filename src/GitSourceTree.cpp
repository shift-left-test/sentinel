/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <algorithm>
#include <cstddef>
#include <filesystem>  // NOLINT
#include <fstream>
#include <sstream>
#include <string>
#include "sentinel/GitSourceTree.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace sentinel {

namespace fs = std::filesystem;

GitSourceTree::GitSourceTree(const std::filesystem::path& baseDirectory) : SourceTree(baseDirectory) {
}

void GitSourceTree::modify(const Mutant& info, const std::filesystem::path& backupPath) {
  // Backup target file to be mutated
  fs::path targetFilename = fs::canonical(getBaseDirectory() / info.getPath());
  fs::path gitRootAbsolutePath = fs::canonical(getBaseDirectory());
  // Component-wise containment check: a raw string startsWith would let
  // siblings whose name shares a prefix (e.g. /repo/foo vs /repo/foobar)
  // bypass the gate. Matches GitRepository::isTargetPath's std::mismatch
  // pattern so both paths agree on what "inside the git root" means.
  auto mm = std::mismatch(gitRootAbsolutePath.begin(), gitRootAbsolutePath.end(),
                          targetFilename.begin(), targetFilename.end());
  if (mm.first != gitRootAbsolutePath.end()) {
    throw IOException(EINVAL, fmt::format("Git root does not contain {}", targetFilename.string()));
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
    throw IOException(EINVAL, fmt::format("Failed to open {}", targetFilename.string()));
  }

  // Detect whether the original file ends with a newline so the same property
  // can be preserved on the mutated file. Sources without a trailing newline
  // are valid in C/C++ and altering this can affect build-system behaviour.
  const std::string originalContent = buffer.str();
  const bool originalEndsWithNewline = !originalContent.empty() && originalContent.back() == '\n';

  std::ofstream mutatedFile(targetFilename.string(), std::ios::trunc);

  // If code line is out of target range, just write to mutant file.
  // If code line is in target range (start_line < code_line < end_line), skip.
  // If code line is on start_line, write the code appearing before start_col,
  // and write mutated token.
  // If code line is on end_line, write the code appearing after end_col.
  std::string line;
  std::size_t lineIdx = 0;
  std::size_t totalLines = std::count(originalContent.begin(), originalContent.end(), '\n');
  // A non-empty file without a trailing newline still has one final line that
  // std::count of '\n' does not include, so account for it here.
  if (!originalContent.empty() && !originalEndsWithNewline) {
    totalLines += 1;
  }

  while (std::getline(buffer, line)) {
    lineIdx += 1;
    const bool isLastLine = (lineIdx == totalLines);
    const bool emitTrailingNewline = !isLastLine || originalEndsWithNewline;

    if (lineIdx < info.getFirst().line || lineIdx > info.getLast().line) {
      mutatedFile << line;
      if (emitTrailingNewline) {
        mutatedFile << '\n';
      }
    }

    if (lineIdx == info.getFirst().line) {
      mutatedFile << line.substr(0, info.getFirst().column - 1);
      mutatedFile << info.getToken();
    }

    if (lineIdx == info.getLast().line) {
      mutatedFile << line.substr(info.getLast().column - 1);
      if (emitTrailingNewline) {
        mutatedFile << '\n';
      }
    }
  }

  mutatedFile.close();
}

}  // namespace sentinel
