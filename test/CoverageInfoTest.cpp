/*
  MIT License

  Copyright (c) 2021 Loc Duy Phan

  Permission is hereby granted,  free of charge,  to any person obtaining a copy
  of this software and associated documentation files (the "Software"),  to deal
  in the Software without restriction,  including without limitation the rights
  to use,  copy,  modify,  merge,  publish,  distribute,  sublicense,  and/or sell
  copies of the Software,  and to permit persons to whom the Software is
  furnished to do so,  subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS",  WITHOUT WARRANTY OF ANY KIND,  EXPRESS OR
  IMPLIED,  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER
  LIABILITY,  WHETHER IN AN ACTION OF CONTRACT,  TORT OR OTHERWISE,  ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <experimental/filesystem>
#include <gtest/gtest.h>
#include <algorithm>
#include "SampleFileGeneratorForTest.hpp"
#include "sentinel/exceptions/IOException.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/CoverageInfo.hpp"

namespace sentinel {

class CoverageInfoTest : public SampleFileGeneratorForTest {
};

TEST_F(CoverageInfoTest, testCoverWorks) {
  std::string filename = SAMPLECOVERAGE_PATH.string();
  std::string targetfile = SAMPLE1_PATH.string();
  CoverageInfo c{std::vector<std::string>(1, filename)};
  EXPECT_FALSE(c.cover("unknown_file", 123));   // nonexist file
  EXPECT_FALSE(c.cover(targetfile, 39));        // uncovered line
  EXPECT_FALSE(c.cover(targetfile, 40));        // uncovered line
  EXPECT_TRUE(c.cover(targetfile, 33));         // covered line
  EXPECT_TRUE(c.cover(targetfile, 35));         // covered line
  EXPECT_FALSE(c.cover(targetfile, 100));        // line not included in file
}

TEST_F(CoverageInfoTest, testFailWhenUnknownFileGiven) {
  EXPECT_THROW(CoverageInfo c{std::vector<std::string>(1, "unknown.info")},
               InvalidArgumentException);
}

}  // namespace sentinel
