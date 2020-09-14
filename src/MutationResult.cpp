/*
  MIT License

  Copyright (c) 2020 Sangmo Kang

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
#include <sys/stat.h>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Mutable.hpp"
#include "sentinel/util/filesystem.hpp"


namespace sentinel {

MutationResult::MutationResult(const sentinel::Mutable& mut,
    const std::string& killingTest, bool detected, int indexOfMutableDB) :
    mKillingTest(killingTest), mDetected(detected),
    mIndexOfMutableDB(indexOfMutableDB), mLineNum(mut.getFirst().line),
    // mMethodDescription(mut.getMethodDescription()),
    // mMutatedClass(mut.getMutatedClass()),
    // mMutatedMethod(mut.getMutatedMethod()),
    mMethodDescription(""), mMutatedClass(""), mMutatedMethod(""),
    mMutator(mut.getOperator()), mPath(mut.getPath()) {
}

MutationResult::MutationResult(const std::string& mutationResultFilePath) {
  if (!util::filesystem::exists(mutationResultFilePath) ||
      util::filesystem::isDirectory(mutationResultFilePath)) {
    throw InvalidArgumentException(fmt::format(
        "mutationResultFilePath doesn't have MutationResult({0})",
        mutationResultFilePath));
  }

  std::ifstream inFile(mutationResultFilePath.c_str());

  mMethodDescription = readStringFromFile(inFile);
  mMutator = readStringFromFile(inFile);
  mMutatedClass = readStringFromFile(inFile);
  mMutatedMethod = readStringFromFile(inFile);
  mPath = readStringFromFile(inFile);
  mKillingTest = readStringFromFile(inFile);

  // avoid no initialization warn
  mDetected = false;
  mLineNum = 0;
  mIndexOfMutableDB = 0;

  inFile.read(reinterpret_cast<char *>(&mDetected),    //NOLINT
      sizeof(mDetected));
  inFile.read(reinterpret_cast<char *>(&mLineNum),    //NOLINT
      sizeof(mLineNum));
  inFile.read(reinterpret_cast<char *>(&mIndexOfMutableDB),    //NOLINT
      sizeof(mIndexOfMutableDB));
  inFile.close();

  struct stat sb = {};
  lstat(mutationResultFilePath.c_str(), &sb);
  mLastModified = sb.st_mtime;
}

std::string MutationResult::getMethodDescription() const {
  return mMethodDescription;
}

std::string MutationResult::getMutator() const {
  return mMutator;
}

std::string MutationResult::getMutatedClass() const {
  return mMutatedClass;
}

std::string MutationResult::getMutatedMethod() const {
  return mMutatedMethod;
}

std::string MutationResult::getPath() const {
  return mPath;
}

std::string MutationResult::getKillingTest() const {
  return mKillingTest;
}

bool MutationResult::getDetected() const {
  return mDetected;
}

int MutationResult::getLineNum() const {
  return mLineNum;
}

int MutationResult::getIndexOfMutableDB() const {
  return mIndexOfMutableDB;
}

std::time_t MutationResult::getLastModifiedTime() const {
  return mLastModified;
}

void MutationResult::saveToFile(const std::string& dirPath) const {
  if (util::filesystem::exists(dirPath)) {
    if (!util::filesystem::isDirectory(dirPath)) {
    throw InvalidArgumentException(fmt::format(
        "dirPath isn't directory({0})", dirPath));
    }
  } else {
    util::filesystem::createDirectory(dirPath);
  }

  std::string filePath = util::filesystem::join(dirPath,
        std::to_string(getIndexOfMutableDB()).append(".MutationResult"));

  std::ofstream outFile(filePath.c_str(),
      std::ios::out | std::ios::binary);

  writeStringToFile(outFile, mMethodDescription);
  writeStringToFile(outFile, mMutator);
  writeStringToFile(outFile, mMutatedClass);
  writeStringToFile(outFile, mMutatedMethod);
  writeStringToFile(outFile, mPath);
  writeStringToFile(outFile, mKillingTest);

  outFile.write(reinterpret_cast<const char *>(&mDetected),    //NOLINT
      sizeof(mDetected));
  outFile.write(reinterpret_cast<const char *>(&mLineNum),    //NOLINT
      sizeof(mLineNum));
  outFile.write(reinterpret_cast<const char *>(&mIndexOfMutableDB),    //NOLINT
      sizeof(mIndexOfMutableDB));

  outFile.close();
}


bool MutationResult::compare(const MutationResult& other) const {
  return mMethodDescription == other.getMethodDescription() &&
      mMutator == other.getMutator() &&
      mMutatedClass == other.getMutatedClass() &&
      mMutatedMethod == other.getMutatedMethod() &&
      mPath == other.getPath() &&
      mKillingTest == other.getKillingTest() &&
      mDetected == other.getDetected() &&
      mLineNum == other.getLineNum() &&
      mIndexOfMutableDB == other.getIndexOfMutableDB();
}

std::string MutationResult::readStringFromFile(std::ifstream& inFile) {
  std::size_t length;
  inFile.read(reinterpret_cast<char *>(&length), sizeof(size_t));    //NOLINT

  char data[length+1];    //NOLINT
  inFile.read(&data[0], length);
  data[length] = '\0';
  std::string res{&data[0]};

  return res;
}

void MutationResult::writeStringToFile(std::ofstream& outFile,
    const std::string& outString) const {
  std::size_t mOutStringSize = outString.size();
  outFile.write(reinterpret_cast<char *>(&mOutStringSize),    //NOLINT
      sizeof(std::size_t));
  outFile.write(outString.c_str(), mOutStringSize);
}

}  // namespace sentinel
