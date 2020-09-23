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

#include <algorithm>
#include <iostream>
#include <random>
#include <vector>
#include "sentinel/Mutables.hpp"
#include "sentinel/UniformMutableSelector.hpp"
#include "sentinel/util/os.hpp"


namespace sentinel {

Mutables UniformMutableSelector::select(const Mutables& mutables,
                                        const SourceLines& sourceLines,
                                        int maxMutables) {
  Mutables temp_storage;
  std::random_device rd;
  std::mt19937 mt(rd());

  for (const auto& line : sourceLines) {
    std::vector<Mutable> temp;

    auto pred = [&](const auto& m) {
      return os::path::comparePath(m.getPath(), line.getPath()) &&
      m.getFirst().line <= line.getLineNumber() &&
      m.getLast().line >= line.getLineNumber();
    };
    std::copy_if(mutables.begin(), mutables.end(),
                 std::back_inserter(temp), pred);

    if (temp.empty()) {
      continue;
    }

    std::uniform_int_distribution<int> idx(0, temp.size()-1);
    temp_storage.push_back(temp[idx(mt)]);
  }

  if (maxMutables >= temp_storage.size()) {
    return Mutables(temp_storage.begin(), temp_storage.end());
  }

  temp_storage.shuffle();
  return Mutables(temp_storage.begin(), temp_storage.begin() + maxMutables);
}

}  // namespace sentinel
