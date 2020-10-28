/*
  MIT License

  Copyright (c) 2020 Sung Gon Kim

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/GitSourceTree.hpp"
#include "sentinel/util/os.hpp"
#include "sentinel/Logger.hpp"


namespace sentinel {
const char * cGitSourceTreeLoggerName = "GitSourceTree";

GitSourceTree::GitSourceTree(const std::string& baseDirectory) :
    SourceTree(baseDirectory) {
}

void GitSourceTree::modify(const Mutant& info, const fs::path& backupPath) {
  // Backup target file to be mutated
  auto logger = Logger::getLogger(cGitSourceTreeLoggerName);

  fs::path targetFilename = fs::canonical(info.getPath());
  fs::path gitRootAbsolutePath = fs::canonical(getBaseDirectory());
  if (!string::startsWith(targetFilename.parent_path(),
                          gitRootAbsolutePath)) {
    throw IOException(EINVAL, "Git root does not contain "
      + targetFilename.string());
  }
  std::string targetRelativePath = \
      targetFilename.parent_path().string().substr(
          gitRootAbsolutePath.string().length());
  fs::path newBackupPath = backupPath / targetRelativePath;
  if (!fs::exists(newBackupPath)) {
    fs::create_directories(newBackupPath);
  }
  fs::copy(targetFilename, newBackupPath,
    fs::copy_options::overwrite_existing);
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
      mutatedFile << line.substr(0, info.getFirst().column-1);
      mutatedFile << info.getToken();
    }

    if (lineIdx == info.getLast().line) {
      mutatedFile << line.substr(info.getLast().column-1) << std::endl;
    }
  }

  mutatedFile.close();
}

}  // namespace sentinel
