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

#include <string>
#include <utility>
#include "sentinel/Mutable.hpp"


namespace sentinel {

Mutable::Mutable(const std::string& mutationOperator,
                 const std::string& path,
                 const Location& first,
                 const Location& last,
                 const std::string& token)
    : mOperator(mutationOperator), mPath(path), mFirst(first),
      mLast(last), mToken(token) {}

Mutable::Mutable(const std::string& mutationOperator,
                 const std::string& path,
                 int firstLine, int firstColumn,
                 int lastLine, int lastColumn,
                 const std::string& token)
    : mOperator(mutationOperator), mPath(path), mFirst{firstLine, firstColumn},
      mLast{lastLine, lastColumn}, mToken(token) {}

bool Mutable::compare(const Mutable& other) const {
  return mOperator == other.getOperator() &&
         mPath == other.getPath() &&
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

Location Mutable::getFirst() const {
  return mFirst;
}

Location Mutable::getLast() const {
  return mLast;
}

std::string Mutable::getToken() const {
  return mToken;
}

}  // namespace sentinel
