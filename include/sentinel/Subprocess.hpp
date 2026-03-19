/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_UTIL_SUBPROCESS_HPP_
#define INCLUDE_SENTINEL_UTIL_SUBPROCESS_HPP_

#include <unistd.h>
#include <filesystem>  // NOLINT
#include <string>

namespace sentinel {

/**
 * @brief Subprocess class
 *
 * @note Only one Subprocess may be active at a time per process.  The signal
 *       handler accesses the static members childPid, killAfter, timedOut and
 *       pendSig; they are declared volatile so that the compiler does not
 *       cache their values across signal-handler boundaries.
 */
class Subprocess {
 public:
  /**
   * @brief Default constructor
   *
   * @param cmd       Shell command to execute.
   * @param sec       Timeout in seconds (0 = no timeout).
   * @param secForKill  Seconds after timeout before SIGKILL (0 = disabled).
   * @param logFile   If non-empty, tee stdout/stderr to this file path.
   * @param silent    If true, suppress stdout/stderr output to terminal (still written to logFile).
   * @throw runtime_error when another Subprocess already running.
   */
  explicit Subprocess(const std::string& cmd, std::size_t sec = 0, std::size_t secForKill = 0,
                      const std::filesystem::path& logFile = "", bool silent = false);

  Subprocess(const Subprocess&) = delete;
  Subprocess& operator=(const Subprocess&) = delete;

  /**
   * @brief execute cmd
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
   * @return 1 if exit and return 0
   */
  bool isSuccessfulExit();

 private:
  std::string mCmd;
  std::size_t mSec;
  std::size_t mSecForKill;
  std::filesystem::path mLogFile;
  bool mSilent = false;
  bool mTimedOut = false;
  int mStatus = -1;
  static volatile pid_t childPid;          ///< PID of the running child process.
  static volatile std::size_t killAfter;   ///< Seconds before SIGKILL after timeout.
  static volatile bool timedOut;           ///< Set to true by the SIGALRM handler.
  static volatile int pendSig;             ///< Pending signal to forward to child.
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_UTIL_SUBPROCESS_HPP_
