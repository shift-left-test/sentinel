/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
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
