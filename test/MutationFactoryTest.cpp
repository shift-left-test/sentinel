/*
  MIT License

  Copyright (c) 2020 Loc Duy Phan

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

#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include "SampleFileGeneratorForTest.hpp"
#include "sentinel/MutationFactory.hpp"
#include "sentinel/UniformMutantGenerator.hpp"
#include "sentinel/util/os.hpp"

namespace sentinel {

class MutationFactoryTest : public SampleFileGeneratorForTest {
};

TEST_F(MutationFactoryTest, testPopulateWorks) {
  SourceLines sourceLines;
  sourceLines.push_back(SourceLine(
      SAMPLE1_PATH, 58));
  sourceLines.push_back(SourceLine(
      SAMPLE1_PATH, 59));

  std::shared_ptr<MutantGenerator> generator =
      std::make_shared<UniformMutantGenerator>("..");
  MutationFactory factory(generator);

  testing::internal::CaptureStdout();
  Mutants selected = factory.populate(SAMPLE1_DIR, sourceLines, 3);
  std::string out = testing::internal::GetCapturedStdout();

  // 1 mutable on line 58, 1 mutable on line 59 are selected
  EXPECT_EQ(selected.size(), 2);
  EXPECT_EQ(selected.at(0).getFirst().line, 58);
  EXPECT_EQ(selected.at(1).getFirst().line, 59);

  EXPECT_TRUE(string::contains(
      out, SAMPLE1_NAME + "                                                2"));
  EXPECT_TRUE(string::contains(
      out, "TOTAL                                                      2"));
}

}  // namespace sentinel
