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

#include <string>
#include "sentinel/Persistence.hpp"


namespace sentinel {

/**
 * @brief Result class
 */
class Result : public Persistence {
 public:
  /**
   * @brief Default constructor
   *
   * @param path to a test result
   */
  explicit Result(const std::string& path);

  /**
   * @brief Compare the current result with the given one
   *
   * @param other result
   * @return true if the both results are identical, false otherwise
   */
  bool equals(const Result& other);
  void load() override;
  void save() override;

 private:
  std::string mPath;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_RESULT_HPP_
