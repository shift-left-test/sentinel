/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "helper/SampleFileGeneratorForTest.hpp"
#include "sentinel/Mutants.hpp"

namespace fs = std::filesystem;

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
    return m1.getOperator() == m2.getOperator() && m1.getPath() == m2.getPath() &&
           m1.getQualifiedFunction() == m2.getQualifiedFunction() && m1.getFirst().line == m2.getFirst().line &&
           m1.getFirst().column == m2.getFirst().column && m1.getLast().line == m2.getLast().line &&
           m1.getLast().column == m2.getLast().column && m1.getToken() == m2.getToken();
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
  EXPECT_THROW(Mutant("AOR", NONEXISTED_FILENAME, "", 0, 0, 0, 0, ONELINE_TOKEN), fs::filesystem_error);
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

TEST_F(MutantsTest, testDefaultConstructorCreatesEmptyMutant) {
  Mutant m;
  EXPECT_EQ("", m.getOperator());
  EXPECT_EQ("", m.getQualifiedFunction());
  EXPECT_EQ("", m.getClass());
  EXPECT_EQ("", m.getFunction());
  EXPECT_EQ(0u, m.getFirst().line);
  EXPECT_EQ(0u, m.getFirst().column);
  EXPECT_EQ(0u, m.getLast().line);
  EXPECT_EQ(0u, m.getLast().column);
  EXPECT_EQ("", m.getToken());
}

TEST_F(MutantsTest, testGetClassAndFunctionWithSimpleName) {
  Mutant m("AOR", NORMAL_FILENAME, "myFunc", 1, 0, 1, 5, "+");
  EXPECT_EQ("", m.getClass());
  EXPECT_EQ("myFunc", m.getFunction());
  EXPECT_EQ("myFunc", m.getQualifiedFunction());
}

TEST_F(MutantsTest, testGetClassAndFunctionWithQualifiedName) {
  // "MyClass::myMethod" → class="MyClass:", function="myMethod"
  Mutant m("AOR", NORMAL_FILENAME, "MyClass::myMethod", 1, 0, 1, 5, "+");
  EXPECT_EQ("MyClass:", m.getClass());
  EXPECT_EQ("myMethod", m.getFunction());
  EXPECT_EQ("MyClass::myMethod", m.getQualifiedFunction());
}

TEST_F(MutantsTest, testGetClassAndFunctionWithDeepQualifiedName) {
  // "A::B::foo" → class="A::B:", function="foo"
  Mutant m("AOR", NORMAL_FILENAME, "A::B::foo", 1, 0, 1, 5, "+");
  EXPECT_EQ("A::B:", m.getClass());
  EXPECT_EQ("foo", m.getFunction());
  EXPECT_EQ("A::B::foo", m.getQualifiedFunction());
}

TEST_F(MutantsTest, testEqualityOperator) {
  Mutant m1("AOR", NORMAL_FILENAME, "foo", 1, 2, 3, 4, "+");
  Mutant m2("AOR", NORMAL_FILENAME, "foo", 1, 2, 3, 4, "+");
  EXPECT_TRUE(m1 == m2);
  EXPECT_FALSE(m1 != m2);
}

TEST_F(MutantsTest, testInequalityOperatorDifferentOperator) {
  Mutant m1("AOR", NORMAL_FILENAME, "foo", 1, 2, 3, 4, "+");
  Mutant m2("BOR", NORMAL_FILENAME, "foo", 1, 2, 3, 4, "+");
  EXPECT_TRUE(m1 != m2);
  EXPECT_FALSE(m1 == m2);
}

TEST_F(MutantsTest, testInequalityOperatorDifferentLocation) {
  Mutant m1("AOR", NORMAL_FILENAME, "foo", 1, 2, 3, 4, "+");
  Mutant m2("AOR", NORMAL_FILENAME, "foo", 5, 6, 7, 8, "+");
  EXPECT_TRUE(m1 != m2);
}

TEST_F(MutantsTest, testLessThanOperator) {
  // operator< compares str() which includes qualifiedFunction field
  Mutant m1("AOR", NORMAL_FILENAME, "aaa", 1, 2, 3, 4, "+");
  Mutant m2("AOR", NORMAL_FILENAME, "zzz", 1, 2, 3, 4, "+");
  EXPECT_TRUE(m1 < m2);
  EXPECT_FALSE(m2 < m1);
}

TEST_F(MutantsTest, testStrContainsExpectedFields) {
  Mutant m("AOR", NORMAL_FILENAME, "foo", 1, 2, 3, 4, "+");
  std::string s = m.str();
  EXPECT_NE(std::string::npos, s.find("AOR,"));
  EXPECT_NE(std::string::npos, s.find(",foo,1,2,3,4,+"));
}

}  // namespace sentinel
