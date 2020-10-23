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

#ifndef INCLUDE_SENTINEL_RESULT_HPP_
#define INCLUDE_SENTINEL_RESULT_HPP_

#include <memory>
#include <string>
#include <vector>
#include "sentinel/Logger.hpp"


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
   * @throw XMLException when loading result xml file fails
   */
  explicit Result(const std::string& path);

  /**
   * @brief Check if mutation is killed
   *
   * @param original result
   * @param mutated result
   * @return killing test case if the mutation is killed (if not, empty string)
   */
  static std::string kill(const Result& original, const Result& mutated);

 private:
  std::vector<std::string> mPassedTC;
  std::shared_ptr<Logger> mLogger;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_RESULT_HPP_
