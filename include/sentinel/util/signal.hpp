/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_UTIL_SIGNAL_HPP_
#define INCLUDE_SENTINEL_UTIL_SIGNAL_HPP_

#include <csignal>
#include <utility>
#include <vector>

namespace sentinel::signal {

/**
 * @brief set signal handler
 *
 * @param signum signal name
 * @param handler signal handler
 */
inline void setSignalHandler(int signum, void (*handler)(int)) {
  struct sigaction target {};
  target.sa_handler = handler;  // NOLINT
  sigemptyset(&target.sa_mask);
  target.sa_flags = 0;
  ::sigaction(signum, &target, nullptr);
}

/**
 * @brief set multiple signal handlers
 *
 * @param setSignum Set of signal names
 * @param handler signal handler
 */
inline void setMultipleSignalHandlers(const std::vector<int>& setSignum, void (*handler)(int)) {
  for (auto target : setSignum) {
    setSignalHandler(target, handler);
  }
}

/**
 * @brief get sigaction
 *
 * @param signum signal name
 * @param [out] current sigaction
 */
inline void getSigaction(int signum, struct sigaction* current) {
  ::sigaction(signum, nullptr, current);
}

/**
 * @brief set sigaction
 *
 * @param signum signal name
 * @param newSigaction
 */
inline void setSigaction(int signum, struct sigaction* newSigaction) {
  ::sigaction(signum, newSigaction, nullptr);
}

/**
 * @brief Sigaction Container
 */
class SaContainer {
 public:
  /**
   * @brief constructor
   *
   * @param signums target signums
   */
  explicit SaContainer(const std::vector<int>& signums) {
    mSignumAndSa.reserve(signums.size());
    for (int signum : signums) {
      struct sigaction sa {};
      getSigaction(signum, &sa);
      mSignumAndSa.emplace_back(signum, sa);
    }
  }

  SaContainer(const SaContainer&) = delete;
  SaContainer& operator=(const SaContainer&) = delete;

  /**
   * @brief destructor
   */
  ~SaContainer() {
    for (auto& [signum, sa] : mSignumAndSa) {
      setSigaction(signum, &sa);
    }
  }

 private:
  std::vector<std::pair<int, struct sigaction>> mSignumAndSa;
};

}  // namespace sentinel::signal

#endif  // INCLUDE_SENTINEL_UTIL_SIGNAL_HPP_
