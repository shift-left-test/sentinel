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
#include "sentinel/util/os.hpp"
#include "sentinel/Mutables.hpp"


namespace sentinel {

static constexpr const char* NONEXISTED_DIR = "nonexist";
static constexpr const char* OUTPUT_PATH = "./mutables.db";
static constexpr const char* NONEXISTED_PATH = "nonexist/mutables.db";
static constexpr const char* NORMAL_FILENAME = "input/sample1/sample1.cpp";
static constexpr const char* NONEXISTED_FILENAME = "nonexist/nonexist.cpp";
static constexpr const char* ABNORMAL_FILENAME = "some,weird\"~ name";
static constexpr const char* ONELINE_TOKEN = "+";
static constexpr const char* MULTILINE_TOKEN = "a + \\\n\tb";
static constexpr const char* EMPTY_TOKEN = "";

class MutablesTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {
    if (os::path::exists(OUTPUT_PATH)) {
      os::removeFile(OUTPUT_PATH);
    }

    if (os::path::exists(NONEXISTED_DIR)) {
      os::removeDirectories(NONEXISTED_DIR);
    }
  }

  bool equal(const Mutable& m1, const Mutable& m2) {
    return m1.getOperator() == m2.getOperator() &&
           m1.getPath() == m2.getPath() &&
           m1.getQualifiedFunction() == m2.getQualifiedFunction() &&
           m1.getFirst().line == m2.getFirst().line &&
           m1.getFirst().column == m2.getFirst().column &&
           m1.getLast().line == m2.getLast().line &&
           m1.getLast().column == m2.getLast().column &&
           m1.getToken() == m2.getToken();
  }
};

TEST_F(MutablesTest, testConstructorFailWhenInvalidDirGiven) {
  EXPECT_THROW(Mutable("AOR", NONEXISTED_FILENAME, "", 0, 0, 0, 0,
                       ONELINE_TOKEN),
               IOException);
}

TEST_F(MutablesTest, testAdd) {
  Mutables m;
  Mutable newMutable("AOR", NORMAL_FILENAME, "main", 0, 0, 0, 0, ONELINE_TOKEN);

  EXPECT_EQ(m.size(), 0);
  m.push_back(newMutable);
  EXPECT_EQ(m.size(), 1);
  EXPECT_TRUE(equal(newMutable, m.at(0)));
}

TEST_F(MutablesTest, testGetFailsWhenGivenIndexOutOfRange) {
  Mutables m;
  EXPECT_THROW(m.at(0), std::out_of_range);
}

TEST_F(MutablesTest, testSaveWorksWhenExistedDirGiven) {
  Mutables m;
  Mutable mutable1("AOR", NORMAL_FILENAME, "foo", 0, 0, 0, 0, ONELINE_TOKEN);
  Mutable mutable2("AOR", NORMAL_FILENAME, "A::foo", 1, 1, 1, 1, EMPTY_TOKEN);
  m.push_back(mutable1);
  m.push_back(mutable2);
  m.save(OUTPUT_PATH);
  EXPECT_TRUE(os::path::exists(OUTPUT_PATH));

  std::ifstream inFile(OUTPUT_PATH);
  Mutable loaded_mutable1;
  EXPECT_NO_THROW(inFile >> loaded_mutable1);
  EXPECT_TRUE(equal(mutable1, loaded_mutable1));

  Mutable loaded_mutable2;
  EXPECT_NO_THROW(inFile >> loaded_mutable2);
  EXPECT_TRUE(equal(mutable2, loaded_mutable2));
  inFile.close();
}

TEST_F(MutablesTest, testSaveWorksWhenNonexistedDirGiven) {
  Mutables m;
  Mutable mutable1("AOR", NORMAL_FILENAME, "main", 0, 0, 0, 0, ONELINE_TOKEN);
  Mutable mutable2("AOR", NORMAL_FILENAME, "A::foo", 1, 1, 1, 1, EMPTY_TOKEN);
  m.push_back(mutable1);
  m.push_back(mutable2);
  m.save(NONEXISTED_PATH);
  EXPECT_TRUE(os::path::exists(NONEXISTED_PATH));

  std::ifstream inFile(NONEXISTED_PATH);
  Mutable loaded_mutable1;
  EXPECT_NO_THROW(inFile >> loaded_mutable1);
  EXPECT_TRUE(equal(mutable1, loaded_mutable1));

  Mutable loaded_mutable2;
  EXPECT_NO_THROW(inFile >> loaded_mutable2);
  EXPECT_TRUE(equal(mutable2, loaded_mutable2));
  inFile.close();
}

TEST_F(MutablesTest, testLoad) {
  Mutables m;
  Mutable mutable1("AOR", NORMAL_FILENAME, "foo", 0, 0, 0, 0, ONELINE_TOKEN);
  Mutable mutable2("AOR", NORMAL_FILENAME, "A::foo", 1, 1, 1, 1, EMPTY_TOKEN);
  m.push_back(mutable1);
  m.push_back(mutable2);
  m.save(OUTPUT_PATH);

  Mutables m2;
  m2.load(OUTPUT_PATH);
  EXPECT_EQ(m2.size(), 2);

  // Could have made a two line EXPECT_TRUE to compare 2 mutables.
  // This is to test ranged-based for loop implementation is working.
  int counter = 0;
  for (const auto& e : m2) {
    EXPECT_TRUE(equal(e, m.at(counter)));
    counter += 1;
  }
}

}  // namespace sentinel
