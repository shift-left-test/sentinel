/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <utility>
#include <vector>
#include "sentinel/Console.hpp"
#include "sentinel/Logger.hpp"
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
  Logger::debug("Signal received: {} ({})", signum, strsignal(signum));
  for (auto& cb : sCallbacks) {
    cb();
  }
  Console::flush();
  if (signum != SIGUSR1) {
    std::exit(EXIT_FAILURE);
  }
}

}  // namespace sentinel
