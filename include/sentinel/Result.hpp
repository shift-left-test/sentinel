/*
  MIT License

  Copyright (c) 2020 LG Electronics, Inc.

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

#ifndef INCLUDE_SENTINEL_RESULT_HPP_
#define INCLUDE_SENTINEL_RESULT_HPP_

#include <memory>
#include <string>
#include <vector>
#include "sentinel/Logger.hpp"
#include "sentinel/MutationState.hpp"


namespace sentinel {

/**
 * @brief Result class
 */
class Result {
 public:
  /**
   * @brief Default constructor
   *
   * @param path to a test result
   */
  explicit Result(const std::string& path);

  /**
   * @brief check if passed testcase doesn't exist
   *
   * @return true if number of passed Testcase is 0
   */
  bool checkPassedTCEmpty();

  /**
   * @brief Check mutation's Result State
   *
   * @param original result
   * @param mutated result
   * @param [out] killingTest
   * @param [out] errorTest
   * @return mutation's Result State 
   */
  static MutationState compare(const Result& original, const Result& mutated,
      std::string* killingTest, std::string* errorTest);

 private:
  std::vector<std::string> mPassedTC;
  std::vector<std::string> mFailedTC;
  std::shared_ptr<Logger> mLogger;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_RESULT_HPP_
