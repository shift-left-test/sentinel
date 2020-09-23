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

#ifndef INCLUDE_SENTINEL_EVALUATOR_HPP_
#define INCLUDE_SENTINEL_EVALUATOR_HPP_

#include <memory>
#include <string>
#include "sentinel/Logger.hpp"
#include "sentinel/Mutables.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Result.hpp"


namespace sentinel {

/**
 * @brief Evaluator class
 */
class Evaluator {
 public:
  /**
   * @brief Default constructor
   *
   * @param mut taget mutable
   * @param expectedResultDir Directory Path of Expected Result
   * @param outDir Directory Path of MutationResult
   */
  Evaluator(const Mutable& mut,
            const std::string& expectedResultDir,
            const std::string& outDir);

  /**
   * @brief Compare an actual with the expected and save&return summary
   *
   * @param ActualResultDir Directory Path of Actural Result
   * @return MutationResult summary of compare
   */
  MutationResult compareAndSaveMutationResult(
      const std::string& ActualResultDir);

 private:
  const Mutable& mMutable;
  std::shared_ptr<Logger> mLogger;
  std::string mOutDir;
  Mutables mMutables;
  Result mExpectedResult;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_EVALUATOR_HPP_
