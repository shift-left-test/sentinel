/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <unistd.h>
#include <csignal>
#include <cstring>
#include <functional>
#include <utility>
#include <vector>
#include "sentinel/Console.hpp"
#include "sentinel/SignalHandler.hpp"
#include "sentinel/util/signal.hpp"

namespace sentinel {

namespace {
std::vector<std::function<void()>> sCallbacks;
}  // namespace

void SignalHandler::clear() {
  sCallbacks.clear();
}

void SignalHandler::add(const std::vector<int>& signals, std::function<void()> callback) {
  sCallbacks.push_back(std::move(callback));
  signal::setMultipleSignalHandlers(signals, dispatch);
}

void SignalHandler::dispatch(int signum) {
  for (const auto& cb : sCallbacks) {
    cb();
  }
  Console::flush();
  if (signum != SIGUSR1) {
    // _exit (not std::exit): we are in a signal context, and std::exit runs
    // atexit handlers and C++ static destructors which are not async-signal-
    // safe. OomHandler uses the same pattern for the SIGUSR1 path; match it.
    _exit(EXIT_FAILURE);
  }
}

}  // namespace sentinel
