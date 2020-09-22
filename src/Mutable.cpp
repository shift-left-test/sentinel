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
#include <iostream>
#include <string>
#include <utility>
#include "sentinel/Mutable.hpp"
#include "sentinel/util/os.hpp"


namespace sentinel {

Mutable::Mutable() : mFirst{0, 0}, mLast{0, 0} {
}

Mutable::Mutable(const std::string& mutationOperator,
                 const std::string& path,
                 const std::string& qualifiedFuncName,
                 int firstLine, int firstColumn,
                 int lastLine, int lastColumn,
                 const std::string& token) :
    mPath(os::path::getAbsolutePath(path)), mToken(token),
    mOperator(mutationOperator), mQualifiedFunction(qualifiedFuncName),
    mFirst{firstLine, firstColumn}, mLast{lastLine, lastColumn} {
  size_t pos = qualifiedFuncName.find_last_of("::");
  if (pos == std::string::npos) {
    mClass = "";
    mFunction = qualifiedFuncName;
  } else {
    mClass = qualifiedFuncName.substr(0, pos);
    mFunction = qualifiedFuncName.substr(pos+2);
  }
}

bool Mutable::compare(const Mutable& other) const {
  return mOperator == other.getOperator() &&
      os::path::comparePath(mPath, other.getPath()) &&
      mFirst.line == other.getFirst().line &&
      mFirst.column == other.getFirst().column &&
      mLast.line == other.getLast().line &&
      mLast.column == other.getLast().column &&
      mToken == other.getToken();
}

std::string Mutable::getOperator() const {
  return mOperator;
}

std::string Mutable::getPath() const {
  return mPath;
}

std::string Mutable::getClass() const {
  return mClass;
}

std::string Mutable::getFunction() const {
  return mFunction;
}

std::string Mutable::getQualifiedFunction() const {
  return mFunction;
}

Location Mutable::getFirst() const {
  return mFirst;
}

Location Mutable::getLast() const {
  return mLast;
}

std::string Mutable::getToken() const {
  return mToken;
}

std::ostream& operator<<(std::ostream& out, const Mutable& m) {
  out << fmt::format("{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}",
                     m.getOperator(),
                     m.getPath(),
                     m.getQualifiedFunction(),
                     m.getFirst().line,
                     m.getFirst().column,
                     m.getLast().line,
                     m.getLast().column,
                     m.getToken());
  return out;
}

std::istream& operator>>(std::istream& in, Mutable &m) {
  std::string line;
  if (getline(in, line)) {
    auto str = string::split(line, "\t");
    m = Mutable(str[0], str[1], str[2], std::stoi(str[3]), std::stoi(str[4]),
                std::stoi(str[5]), std::stoi(str[6]), str[7]);
  }
  return in;
}

}  // namespace sentinel
