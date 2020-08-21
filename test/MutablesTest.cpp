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

#include <gtest/gtest.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "sentinel/util/filesystem.hpp"
#include "sentinel/Mutables.hpp"


namespace sentinel {

static constexpr const char* OUTPUT_FILENAME = "mutables.db";
static constexpr const char* NORMAL_FILENAME = "temp.cpp";
static constexpr const char* ABNORMAL_FILENAME = "some,weird\"~ name";
static constexpr const char* ONELINE_TOKEN = "+";
static constexpr const char* MULTILINE_TOKEN = "a + \\\n\tb";
static constexpr const char* EMPTY_TOKEN = "";

class MutablesTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {
    if (util::filesystem::exists(OUTPUT_FILENAME)) {
      util::filesystem::removeFile(OUTPUT_FILENAME);
    }
  }
};

TEST_F(MutablesTest, testAdd) {
  Mutables m{OUTPUT_FILENAME};
  Mutable newMutable(NORMAL_FILENAME, 0, 0, 0, 0, ONELINE_TOKEN);

  EXPECT_EQ(m.size(), 0);
  m.add(newMutable);
  EXPECT_EQ(m.size(), 1);
  EXPECT_TRUE(newMutable.compare(m.get(0)));
}

TEST_F(MutablesTest, testGetFailsWhenGivenIndexOutOfRange) {
  Mutables m{OUTPUT_FILENAME};
  EXPECT_THROW(m.get(0), std::out_of_range);
}

TEST_F(MutablesTest, testSave) {
  Mutables m{OUTPUT_FILENAME};
  Mutable mutable1(NORMAL_FILENAME, 0, 0, 0, 0, MULTILINE_TOKEN);
  Mutable mutable2(ABNORMAL_FILENAME, 1, 1, 1, 1, EMPTY_TOKEN);
  m.add(mutable1);
  m.add(mutable2);
  m.save();
  EXPECT_TRUE(util::filesystem::exists(OUTPUT_FILENAME));

  std::ifstream inFile(OUTPUT_FILENAME);
  EXPECT_EQ(Mutables::readIntFromFile(inFile), 2);

  std::string path = Mutables::readStringFromFile(inFile);
  std::string token = Mutables::readStringFromFile(inFile);
  int firstLine = Mutables::readIntFromFile(inFile);
  int firstColumn = Mutables::readIntFromFile(inFile);
  int lastLine = Mutables::readIntFromFile(inFile);
  int lastColumn = Mutables::readIntFromFile(inFile);
  EXPECT_TRUE(mutable1.compare(Mutable(path, firstLine, firstColumn,
                                       lastLine, lastColumn, token)));

  path = Mutables::readStringFromFile(inFile);
  token = Mutables::readStringFromFile(inFile);
  firstLine = Mutables::readIntFromFile(inFile);
  firstColumn = Mutables::readIntFromFile(inFile);
  lastLine = Mutables::readIntFromFile(inFile);
  lastColumn = Mutables::readIntFromFile(inFile);
  EXPECT_TRUE(mutable2.compare(Mutable(path, firstLine, firstColumn,
                                       lastLine, lastColumn, token)));
  inFile.close();
}

TEST_F(MutablesTest, testLoad) {
  Mutables m{OUTPUT_FILENAME};
  Mutable mutable1(NORMAL_FILENAME, 0, 0, 0, 0, MULTILINE_TOKEN);
  Mutable mutable2(ABNORMAL_FILENAME, 1, 1, 1, 1, EMPTY_TOKEN);
  m.add(mutable1);
  m.add(mutable2);
  m.save();

  Mutables m2{OUTPUT_FILENAME};
  m2.load();
  EXPECT_EQ(m2.size(), 2);

  // Could have made a two line EXPECT_TRUE to compare 2 mutables.
  // This is to test ranged-based for loop implementation is working.
  int counter = 0;
  for (const auto& e : m2) {
    EXPECT_TRUE(e.compare(m.get(counter)));
    counter += 1;
  }
}

}  // namespace sentinel
