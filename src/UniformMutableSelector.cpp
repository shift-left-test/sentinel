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
  Mutables ret{mutables.getPath()};
  std::vector<Mutable> temp_storage;
  std::random_device rd;
  std::mt19937 mt(rd());

  for (const auto& line : sourceLines) {
    std::vector<Mutable> temp;
    for (int i = 0; i < mutables.size(); ++i) {
      Mutable m = mutables.get(i);
      if (os::path::comparePath(m.getPath(), line.getPath()) &&
          m.getFirst().line <= line.getLineNumber() &&
          m.getLast().line >= line.getLineNumber()) {
        temp.push_back(m);
      }
    }

    if (temp.empty()) {
      continue;
    }

    // std::cout << line.getLineNumber() << " has " << temp.size() << std::endl;

    std::uniform_int_distribution<int> idx(0, temp.size());
    temp_storage.push_back(temp[0]);
    // temp_storage.push_back(temp[idx(mt)]);
  }

  if (maxMutables >= temp_storage.size()) {
    // std::cout << "Not exceeded" << std::endl;
    for (const auto& e : temp_storage) {
      ret.add(e);
    }
  } else {
    std::shuffle(temp_storage.begin(), temp_storage.end(), mt);
    for (int i = 0; i < maxMutables; ++i) {
      ret.add(temp_storage[i]);
    }
  }

  return ret;
}

}  // namespace sentinel
