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
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include "sentinel/Mutable.hpp"
#include "sentinel/Mutables.hpp"
#include "sentinel/util/filesystem.hpp"
#include "sentinel/exceptions/IOException.hpp"


namespace sentinel {

Mutables::Mutables(const std::string& path) : mPath(path) {}

void Mutables::add(const Mutable& m) {
  mData.push_back(m);
}

Mutable Mutables::get(std::size_t index) {
  if (index >= size()) {
    throw std::out_of_range("Mutables: index out of range");
  }

  return mData[index];
}

int Mutables::size() { return mData.size(); }

void Mutables::load() {
  if (!util::filesystem::exists(mPath) ||
      util::filesystem::isDirectory(mPath)) {
    throw IOException(EINVAL);
  }

  std::ifstream inFile("example.txt");
  std::string path, firstLine, firstCol, lastLine, lastCol, token;

  while (getline(inFile, path, '\0')) {
    // read Mutable information.
    if (getline(inFile, firstLine, '\0')) {
      throw IOException(EINVAL, "load: Insufficient Mutable information");
    }

    if (getline(inFile, firstCol, '\0')) {
      throw IOException(EINVAL, "load: Insufficient Mutable information");
    }

    if (getline(inFile, lastLine, '\0')) {
      throw IOException(EINVAL, "load: Insufficient Mutable information");
    }

    if (getline(inFile, lastCol, '\0')) {
      throw IOException(EINVAL, "load: Insufficient Mutable information");
    }

    if (getline(inFile, token, '\0')) {
      throw IOException(EINVAL, "load: Insufficient Mutable information");
    }

    mData.emplace_back(Mutable(path, std::stoi(firstLine), std::stoi(firstCol),
                               std::stoi(lastLine), std::stoi(lastCol), token));

    // read newline character and move on to the next Mutable on next line.
    getline(inFile, path);
  }

  inFile.close();
}

void Mutables::save() {
  std::ofstream outFile(mPath.c_str());

  for (const auto& e : mData) {
    Location first = e.getFirst();
    Location last = e.getLast();
    std::string mutableFormat{
        "{1}\0{2}\0{3}\0{4}\0{5}\0{6}\0\n"};  // NOLINT
    outFile << fmt::format(mutableFormat,
                           fmt::arg("1", e.getPath()),
                           fmt::arg("2", first.line),
                           fmt::arg("3", first.column),
                           fmt::arg("4", last.line),
                           fmt::arg("5", last.column),
                           fmt::arg("6", e.getToken()));
  }

  outFile.close();
}

}  // namespace sentinel
