/*
  MIT License

  Copyright (c) 2020 LG Electronics, Inc.

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
#include <experimental/filesystem>
#include <iostream>
#include <string>
#include <utility>
#include "sentinel/Mutant.hpp"


namespace sentinel {

Mutant::Mutant() : mFirst{0, 0}, mLast{0, 0} {
}

Mutant::Mutant(const std::string& mutationOperator,
                 const std::string& path,
                 const std::string& qualifiedFuncName,
                 std::size_t firstLine, std::size_t firstColumn,
                 std::size_t lastLine, std::size_t lastColumn,
                 const std::string& token) :
    mPath(std::experimental::filesystem::canonical(path)), mToken(token),
    mOperator(mutationOperator), mQualifiedFunction(qualifiedFuncName),
    mFirst{firstLine, firstColumn}, mLast{lastLine, lastColumn} {
  std::size_t pos = qualifiedFuncName.find_last_of(':');
  if (pos == std::string::npos) {
    mClass = "";
    mFunction = qualifiedFuncName;
  } else {
    mClass = qualifiedFuncName.substr(0, pos);
    mFunction = qualifiedFuncName.substr(pos+1);
  }
}

bool Mutant::operator==(const Mutant& other) const {
  namespace fs = std::experimental::filesystem;

  return mOperator == other.getOperator() &&
      fs::equivalent(mPath, other.getPath()) &&
      mFirst.line == other.getFirst().line &&
      mFirst.column == other.getFirst().column &&
      mLast.line == other.getLast().line &&
      mLast.column == other.getLast().column &&
      mToken == other.getToken();
}

bool Mutant::operator!=(const Mutant& other) const {
  return !(*this == other);
}

std::string Mutant::getOperator() const {
  return mOperator;
}

std::experimental::filesystem::path Mutant::getPath() const {
  return mPath;
}

std::string Mutant::getClass() const {
  return mClass;
}

std::string Mutant::getFunction() const {
  return mFunction;
}

std::string Mutant::getQualifiedFunction() const {
  return mFunction;
}

Location Mutant::getFirst() const {
  return mFirst;
}

Location Mutant::getLast() const {
  return mLast;
}

std::string Mutant::getToken() const {
  return mToken;
}

std::string Mutant::str() const {
  return fmt::format("{},{},{},{},{},{},{},{}", getOperator(),
      getPath().string(), getQualifiedFunction(),
      getFirst().line, getFirst().column, getLast().line, getLast().column,
      getToken());
}

std::ostream& operator<<(std::ostream& out, const Mutant& m) {
  out << m.str();
  return out;
}

std::istream& operator>>(std::istream& in, Mutant &m) {
  std::string line;
  if (getline(in, line)) {
    auto str = string::split(line, ",");
    m = Mutant(str[0], str[1], str[2], std::stoi(str[3]), std::stoi(str[4]),
                std::stoi(str[5]), std::stoi(str[6]), str[7]);
  }
  return in;
}

}  // namespace sentinel
