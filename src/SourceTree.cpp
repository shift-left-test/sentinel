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

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/Mutable.hpp"
#include "sentinel/SourceTree.hpp"
#include "sentinel/util/filesystem.hpp"


namespace sentinel {

void SourceTree::modify(const Mutable& info,
                        const std::string& gitRootPath,
                        const std::string& backupPath) {
  if (!util::filesystem::exists(gitRootPath) ||
      !util::filesystem::exists(backupPath)) {
    throw IOException(EINVAL);
  }

  // backup
  std::string targetFilename = \
      util::filesystem::getAbsolutePath(info.getPath());
  std::string gitRootAbsolutePath = \
      util::filesystem::getAbsolutePath(gitRootPath);
  if (!util::string::startsWith(util::filesystem::dirname(targetFilename),
                                gitRootAbsolutePath)) {
    throw IOException(EINVAL, "Git root does not contain " + targetFilename);
  }
  std::string targetRelativePath = \
      util::filesystem::dirname(targetFilename).substr(
          gitRootAbsolutePath.length());
  std::string newBackupPath = backupPath + "/" + targetRelativePath;
  if (!util::filesystem::exists(newBackupPath)) {
    util::filesystem::createDirectories(newBackupPath);
  }
  util::filesystem::copyFile(targetFilename, newBackupPath);

  // mutation
  std::ifstream originalFile(targetFilename);
  std::stringstream buffer;
  if (originalFile) {
    buffer << originalFile.rdbuf();
    originalFile.close();
  } else {
    throw IOException(EINVAL, "Fail to open " + targetFilename);
  }
  std::ofstream mutatedFile(targetFilename, std::ios::trunc);

  // If code line is out of target range, just write to mutant file.
  // If code line is in target range (start_line < code_line < end_line), skip.
  // If code line is on start_line, write the code appearing before start_col,
  // and write mutated token.
  // If code line is on end_line, write the code appearing after end_col.
  std::string line;
  int lineIdx = 0;

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

std::string SourceTree::toString() {
  return "";
}

}  // namespace sentinel
