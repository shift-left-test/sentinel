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
#include <iostream>
#include <stdexcept>
#include <string>
#include "sentinel/Mutable.hpp"
#include "sentinel/Mutables.hpp"
#include "sentinel/util/os.hpp"
#include "sentinel/exceptions/IOException.hpp"


namespace sentinel {

Mutables::Mutables(const std::string& path) : mPath(path) {}

void Mutables::add(const Mutable& m) {
  mData.push_back(m);
}

Mutable Mutables::get(std::size_t index) const {
  if (index >= size()) {
    throw std::out_of_range("Mutables: index out of range");
  }

  return mData[index];
}

std::string Mutables::getPath() const {
  return mPath;
}

int Mutables::size() const {
  return mData.size();
}

std::vector<Mutable>::const_iterator Mutables::begin() const {
  return mData.begin();
}

std::vector<Mutable>::const_iterator Mutables::end() const {
  return mData.end();
}

std::vector<Mutable>::const_iterator Mutables::cbegin() const {
  return mData.cbegin();
}

std::vector<Mutable>::const_iterator Mutables::cend() const {
  return mData.cend();
}

void Mutables::load() {
  if (!os::path::exists(mPath) ||
      os::path::isDirectory(mPath)) {
    throw IOException(EINVAL);
  }

  std::ifstream inFile(mPath.c_str());
  int num_mutants = readIntFromFile(inFile);

  for (int i = 0; i < num_mutants; ++i) {
    std::string path = readStringFromFile(inFile);
    std::string func = readStringFromFile(inFile);
    std::string mutationOperator = readStringFromFile(inFile);
    std::string token = readStringFromFile(inFile);
    int firstLine = readIntFromFile(inFile);
    int firstColumn = readIntFromFile(inFile);
    int lastLine = readIntFromFile(inFile);
    int lastColumn = readIntFromFile(inFile);
    mData.emplace_back(Mutable(mutationOperator, path, func, firstLine,
                               firstColumn, lastLine, lastColumn, token));
  }

  inFile.close();
}

void Mutables::save() {
  std::string dbDir = os::path::dirname(mPath);
  if (!os::path::exists(dbDir)) {
    os::createDirectories(dbDir);
  }

  std::ofstream outFile(mPath.c_str(), std::ios::out | std::ios::binary);
  if (!outFile) {
    throw IOException(EBADF, "Fail to open mutable_db");
  }

  int num_mutants = mData.size();
  outFile.write(
        reinterpret_cast<char *>(&num_mutants), sizeof(int));      //NOLINT

  for (const auto& e : mData) {
    Location first = e.getFirst();
    Location last = e.getLast();
    std::string mutationOperator = e.getOperator();
    std::string path = e.getPath();
    std::string func = e.getQualifiedFunction();
    std::string token = e.getToken();
    size_t operatorLength = mutationOperator.size();
    size_t pathLength = path.size();
    size_t funcLength = func.size();
    size_t tokenLength = token.size();

    outFile.write(
        reinterpret_cast<char *>(&pathLength), sizeof(size_t));   //NOLINT
    outFile.write(path.c_str(), pathLength);
    outFile.write(
        reinterpret_cast<char *>(&funcLength), sizeof(size_t));   //NOLINT
    outFile.write(func.c_str(), funcLength);
    outFile.write(
        reinterpret_cast<char *>(&operatorLength), sizeof(size_t));   //NOLINT
    outFile.write(mutationOperator.c_str(), operatorLength);
    outFile.write(
        reinterpret_cast<char *>(&tokenLength), sizeof(size_t));  //NOLINT
    outFile.write(token.c_str(), tokenLength);
    outFile.write(
        reinterpret_cast<char *>(&first.line), sizeof(int));      //NOLINT
    outFile.write(
        reinterpret_cast<char *>(&first.column), sizeof(int));    //NOLINT
    outFile.write(
        reinterpret_cast<char *>(&last.line), sizeof(int));       //NOLINT
    outFile.write(
        reinterpret_cast<char *>(&last.column), sizeof(int));     //NOLINT
  }

  outFile.close();
}

std::string Mutables::readStringFromFile(std::ifstream& inFile) {
  size_t length;
  inFile.read(reinterpret_cast<char *>(&length), sizeof(size_t));  //NOLINT

  char data[length+1];  //NOLINT
  inFile.read(&data[0], length);
  data[length] = '\0';
  std::string res{&data[0]};

  return res;
}

int Mutables::readIntFromFile(std::ifstream& inFile) {
  int res;
  inFile.read(reinterpret_cast<char *>(&res), sizeof(int));  //NOLINT
  return res;
}

}  // namespace sentinel
