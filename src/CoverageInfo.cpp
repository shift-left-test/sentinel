/*
  MIT License

  Copyright (c) 2021 Loc Duy Phan

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

#include <experimental/filesystem>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "sentinel/CoverageInfo.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

CoverageInfo::CoverageInfo(const std::vector<std::string>& filenames) {
  namespace fs = std::experimental::filesystem;
  for (const auto& filename : filenames) {
    std::ifstream coverageFile(filename.c_str());
    std::string line;
    std::string currentFile;

    while (std::getline(coverageFile, line)) {
      if (line.substr(0, 2) == "SF") {
        std::vector<std::string> v = string::split(line, ':');
        fs::path p = fs::absolute(fs::path(v[1]));
        mData[p.string()] = std::vector<size_t>();
        currentFile = p.string();
      }

      if (line.substr(0, 2) == "DA") {
        std::vector<std::string> v1 = string::split(line, ':');
        std::vector<std::string> v2 = string::split(v1[1], ',');
        if (v2[1] != "0") {
          mData[currentFile].push_back(std::stoi(v2[0]));
        }
      }
    }
  }
}

bool CoverageInfo::cover(const std::string& filename, size_t line) {
  namespace fs = std::experimental::filesystem;
  fs::path p = fs::absolute(fs::path(filename));

  if (mData.find(p.string()) == mData.end()) {
    return false;
  }

  return std::find(mData[p.string()].begin(),
                   mData[p.string()].end(), line) != mData[p.string()].end();
}

}  // namespace sentinel
