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
#include "sentinel/Logger.hpp"
#include "sentinel/Mutable.hpp"
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
   * @param logger object
   * @param info mutable information
   */
  Evaluator(const std::shared_ptr<Logger>& logger, const Mutable& info);

  /**
   * @brief Compare an actual result with the expected one and return the result
   *
   * @param expected result
   * @param actual result
   * @return summary for the mutation report
   */
  MutationResult compare(const Result& expected, const Result& actual);

 private:
  std::shared_ptr<Logger> mLogger;
  Mutable mMutable;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_EVALUATOR_HPP_
