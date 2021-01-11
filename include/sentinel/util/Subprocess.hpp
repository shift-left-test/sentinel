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

#ifndef INCLUDE_SENTINEL_UTIL_SUBPROCESS_HPP_
#define INCLUDE_SENTINEL_UTIL_SUBPROCESS_HPP_

#include <unistd.h>
#include <string>


namespace sentinel {

/**
 * @brief Subprocess class
 */
class Subprocess {
 public:
  /**
   * @brief Default constructor
   *
   * @param cmd to excute
   * @param sec for timeout
   * @param secForKill
   * @throw runtime_error when another Subprocess already running.
   */
  explicit Subprocess(const std::string& cmd, std::size_t sec = 0,
      std::size_t secForKill = 0);

  /**
   * @brief excute cmd
   *
   * @return exit status
   * @throw runtime_error when bin/sh doesn't exist or fork fail.
   */
  int execute();

  /**
   * @brief check if timeout occurs
   *
   * @return 1 if timeout occurs
   */
  bool isTimedOut();

  /**
   * @brief check if exit and return 0
   *
   * @return 1 if exit and retun 0
   */
  bool isSuccessfulExit();

 private:
  std::string mCmd;
  std::size_t mSec;
  std::size_t mSecForKill;
  bool mTimedOut = false;
  int mStatus = -1;
  static pid_t childPid;
  static std::size_t killAfter;
  static bool timedOut;
  static int pendSig;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_UTIL_SUBPROCESS_HPP_
