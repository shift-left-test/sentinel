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
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/util/filesystem.hpp"
#include "sentinel/exceptions/IOException.hpp"


namespace sentinel {

MutationResults::MutationResults(const std::string& path) : mPath(path) {
  if (util::filesystem::exists(mPath)) {
    if (!util::filesystem::isDirectory(mPath)) {
      throw InvalidArgumentException(fmt::format("path isn't directory({0})",
          path));
    }
  } else {
    util::filesystem::createDirectory(mPath);
  }
}

void MutationResults::add(const MutationResult& m) {
  mData.push_back(m);
}

MutationResult MutationResults::get(std::size_t index) {
  if (index >= size()) {
    throw std::out_of_range("MutationResults: index out of range");
  }

  return mData[index];
}

int MutationResults::size() { return mData.size(); }

void MutationResults::load() {
  auto mutationResultFiles = util::filesystem::findFilesInDirUsingExt(mPath,
      {"MutationResult"});
  if (mutationResultFiles.empty()) {
    throw InvalidArgumentException(fmt::format("path is empty({0})",
        mPath));
  }
  std::transform(mutationResultFiles.begin(), mutationResultFiles.end(),
      std::back_inserter(mData), [] (const std::string& path)
      -> MutationResult { return MutationResult(path); } );
  sortByIndexOfMutableDB();
}

void MutationResults::sortByIndexOfMutableDB() {
  std::sort(mData.begin(), mData.end(), [](const auto& lhs, const auto& rhs)
      { return rhs.getIndexOfMutableDB() > lhs.getIndexOfMutableDB(); } );
}

void MutationResults::save() {
  for (const auto& e : mData) {
    e.saveToFile(mPath);
  }
}

}  // namespace sentinel

