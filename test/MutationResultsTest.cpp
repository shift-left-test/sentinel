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

#include <gtest/gtest.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "sentinel/MutationResults.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Mutable.hpp"
#include "sentinel/util/filesystem.hpp"


namespace sentinel {

class MutationResultsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    BASE = util::filesystem::tempDirectory("fixture");
    OUT_DIR = util::filesystem::tempDirectory(util::filesystem::join(BASE,
        "ORI_DIR"));
  }

  void TearDown() override {
    util::filesystem::removeDirectories(BASE);
  }

  std::string BASE;
  std::string OUT_DIR;
};

TEST_F(MutationResultsTest, testAdd) {
  MutationResults MRs(OUT_DIR);
  Mutable M1("AOR", "Test.cpp", 0, 0, 0, 0, "+");
  MutationResult MR1(M1, "testAdd", true, 0);
  EXPECT_EQ(0, MRs.size());

  MRs.add(MR1);
  EXPECT_EQ(1, MRs.size());

  EXPECT_TRUE(MRs.get(0).compare(MR1));
  EXPECT_EQ(MRs.get(0).getMutator(), "AOR");
  EXPECT_EQ(MR1.getPath(), "Test.cpp");
  EXPECT_EQ(MR1.getLineNum(), 0);
}

TEST_F(MutationResultsTest, testGetFailsWhenGivenIndexOutOfRange) {
  MutationResults m{OUT_DIR};
  EXPECT_THROW(m.get(0), std::out_of_range);
}

TEST_F(MutationResultsTest, testSaveAndLoad) {
  MutationResults MRs(OUT_DIR);

  Mutable M1("AOR", "Test.cpp", 4, 5, 6, 7, "+");
  MutationResult MR1(M1, "testAdd", false, 0);
  MRs.add(MR1);

  Mutable M2("BOR", "Test2.cpp", 1, 2, 3, 4, "|");
  MutationResult MR2(M2, "testAddBit", true, 1);
  MRs.add(MR2);

  MRs.save();
  EXPECT_TRUE(util::filesystem::exists(util::filesystem::join(
      OUT_DIR, "0.MutationResult")));
  EXPECT_TRUE(util::filesystem::exists(util::filesystem::join(
      OUT_DIR, "1.MutationResult")));

  MutationResults MRs2(OUT_DIR);
  MRs2.load();

  EXPECT_TRUE(MRs2.get(0).compare(MR1));
  EXPECT_TRUE(MRs2.get(1).compare(MR2));
}

}  // namespace sentinel
