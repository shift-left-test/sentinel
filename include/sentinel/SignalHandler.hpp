/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_SIGNALHANDLER_HPP_
#define INCLUDE_SENTINEL_SIGNALHANDLER_HPP_

#include <functional>
#include <vector>

namespace sentinel {

/**
 * @brief Installs signal handlers and dispatches to registered callbacks.
 *
 * NOTE: callbacks are invoked from a signal handler context and are therefore
 * not strictly async-signal-safe. This is an accepted trade-off since the
 * codebase already uses non-async-safe operations (Console, filesystem) in
 * signal handlers.
 */
class SignalHandler {
 public:
  /**
   * @brief Register a cleanup callback and install it for the given signals.
   *        May be called multiple times; each call appends a callback and
   *        (re-)installs the dispatcher for the specified signals.
   *
   * @param signals  Signal numbers to handle.
   * @param callback Cleanup function to invoke when a signal is received.
   */
  static void add(const std::vector<int>& signals, std::function<void()> callback);

  /**
   * @brief Remove all registered callbacks.
   *        Primarily for use in tests to ensure isolation between test cases.
   */
  static void clear();

 private:
  static void dispatch(int signum);
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_SIGNALHANDLER_HPP_
