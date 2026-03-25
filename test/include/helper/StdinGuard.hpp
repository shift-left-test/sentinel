/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef TEST_INCLUDE_HELPER_STDINGUARD_HPP_
#define TEST_INCLUDE_HELPER_STDINGUARD_HPP_

#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

namespace sentinel {

/**
 * @brief RAII guard that redirects stdin to /dev/null.
 *
 * On construction, redirects STDIN_FILENO to /dev/null.
 * On destruction, restores the original stdin fd.
 */
class StdinGuard {
 public:
  StdinGuard() : mSavedFd(dup(STDIN_FILENO)) {
    if (mSavedFd < 0) {
      throw std::runtime_error("StdinGuard: failed to dup stdin");
    }
    int devNull = open("/dev/null", O_RDONLY);
    if (devNull < 0) {
      close(mSavedFd);
      throw std::runtime_error("StdinGuard: failed to open /dev/null");
    }
    dup2(devNull, STDIN_FILENO);
    close(devNull);
  }

  ~StdinGuard() {
    dup2(mSavedFd, STDIN_FILENO);
    close(mSavedFd);
  }

  StdinGuard(const StdinGuard&) = delete;
  StdinGuard& operator=(const StdinGuard&) = delete;

 private:
  int mSavedFd;
};

}  // namespace sentinel

#endif  // TEST_INCLUDE_HELPER_STDINGUARD_HPP_
