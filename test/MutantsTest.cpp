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
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "SampleFileGeneratorForTest.hpp"
#include "sentinel/Mutants.hpp"


namespace fs = std::experimental::filesystem;

namespace sentinel {

class MutantsTest : public SampleFileGeneratorForTest {
 protected:
  void SetUp() override {
    SampleFileGeneratorForTest::SetUp();
    NORMAL_FILENAME = SAMPLE1_PATH;
  }

  void TearDown() override {
    if (fs::exists(OUTPUT_PATH)) {
      fs::remove(OUTPUT_PATH);
    }

    if (fs::exists(NONEXISTED_DIR)) {
      fs::remove_all(NONEXISTED_DIR);
    }
    SampleFileGeneratorForTest::TearDown();
  }

  bool equal(const Mutant& m1, const Mutant& m2) {
    return m1.getOperator() == m2.getOperator() &&
           m1.getPath() == m2.getPath() &&
           m1.getQualifiedFunction() == m2.getQualifiedFunction() &&
           m1.getFirst().line == m2.getFirst().line &&
           m1.getFirst().column == m2.getFirst().column &&
           m1.getLast().line == m2.getLast().line &&
           m1.getLast().column == m2.getLast().column &&
           m1.getToken() == m2.getToken();
  }

  std::string NONEXISTED_DIR = "nonexist";
  std::string OUTPUT_PATH = "./mutables.db";
  std::string NONEXISTED_PATH = "nonexist/mutables.db";
  std::string NORMAL_FILENAME;
  std::string NONEXISTED_FILENAME = "nonexist/nonexist.cpp";
  std::string ABNORMAL_FILENAME = "some,weird\"~ name";
  std::string ONELINE_TOKEN = "+";
  std::string MULTILINE_TOKEN = "a + \\\n\tb";
  std::string EMPTY_TOKEN = "";
};

TEST_F(MutantsTest, testConstructorFailWhenInvalidDirGiven) {
  EXPECT_THROW(Mutant("AOR", NONEXISTED_FILENAME, "", 0, 0, 0, 0,
                       ONELINE_TOKEN),
               fs::filesystem_error);
}

TEST_F(MutantsTest, testAdd) {
  Mutants m;
  Mutant newMutant("AOR", NORMAL_FILENAME, "main", 0, 0, 0, 0, ONELINE_TOKEN);

  EXPECT_EQ(m.size(), 0);
  m.push_back(newMutant);
  EXPECT_EQ(m.size(), 1);
  EXPECT_TRUE(equal(newMutant, m.at(0)));
}

TEST_F(MutantsTest, testGetFailsWhenGivenIndexOutOfRange) {
  Mutants m;
  EXPECT_THROW(m.at(0), std::out_of_range);
}

TEST_F(MutantsTest, testSaveWorksWhenExistedDirGiven) {
  Mutants m;
  Mutant mutable1("AOR", NORMAL_FILENAME, "foo", 0, 0, 0, 0, ONELINE_TOKEN);
  Mutant mutable2("AOR", NORMAL_FILENAME, "A::foo", 1, 1, 1, 1, EMPTY_TOKEN);
  m.push_back(mutable1);
  m.push_back(mutable2);
  m.save(OUTPUT_PATH);
  EXPECT_TRUE(fs::exists(OUTPUT_PATH));

  std::ifstream inFile(OUTPUT_PATH);
  Mutant loaded_mutable1;
  EXPECT_NO_THROW(inFile >> loaded_mutable1);
  EXPECT_TRUE(equal(mutable1, loaded_mutable1));

  Mutant loaded_mutable2;
  EXPECT_NO_THROW(inFile >> loaded_mutable2);
  EXPECT_TRUE(equal(mutable2, loaded_mutable2));
  inFile.close();
}

TEST_F(MutantsTest, testSaveWorksWhenNonexistedDirGiven) {
  Mutants m;
  Mutant mutable1("AOR", NORMAL_FILENAME, "main", 0, 0, 0, 0, ONELINE_TOKEN);
  Mutant mutable2("AOR", NORMAL_FILENAME, "A::foo", 1, 1, 1, 1, EMPTY_TOKEN);
  m.push_back(mutable1);
  m.push_back(mutable2);
  m.save(NONEXISTED_PATH);
  EXPECT_TRUE(fs::exists(NONEXISTED_PATH));

  std::ifstream inFile(NONEXISTED_PATH);
  Mutant loaded_mutable1;
  EXPECT_NO_THROW(inFile >> loaded_mutable1);
  EXPECT_TRUE(equal(mutable1, loaded_mutable1));

  Mutant loaded_mutable2;
  EXPECT_NO_THROW(inFile >> loaded_mutable2);
  EXPECT_TRUE(equal(mutable2, loaded_mutable2));
  inFile.close();
}

TEST_F(MutantsTest, testLoad) {
  Mutants m;
  Mutant mutable1("AOR", NORMAL_FILENAME, "foo", 0, 0, 0, 0, ONELINE_TOKEN);
  Mutant mutable2("AOR", NORMAL_FILENAME, "A::foo", 1, 1, 1, 1, EMPTY_TOKEN);
  m.push_back(mutable1);
  m.push_back(mutable2);
  m.save(OUTPUT_PATH);

  Mutants m2;
  m2.load(OUTPUT_PATH);
  EXPECT_EQ(m2.size(), 2);

  // Could have made a two line EXPECT_TRUE to compare 2 mutables.
  // This is to test ranged-based for loop implementation is working.
  std::size_t counter = 0;
  for (const auto& e : m2) {
    EXPECT_TRUE(equal(e, m.at(counter)));
    counter += 1;
  }
}

}  // namespace sentinel
