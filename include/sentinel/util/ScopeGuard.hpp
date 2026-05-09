/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */
#ifndef INCLUDE_SENTINEL_UTIL_SCOPEGUARD_HPP_
#define INCLUDE_SENTINEL_UTIL_SCOPEGUARD_HPP_

#include <utility>

namespace sentinel {

/// RAII scope guard: invokes fn on destruction.
template <typename F>
class ScopeGuard {
 public:
  /// @param fn callable invoked on destruction
  explicit ScopeGuard(F fn) : mFn(std::move(fn)) {}
  ~ScopeGuard() { mFn(); }
  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard& operator=(const ScopeGuard&) = delete;
  ScopeGuard(ScopeGuard&&) = delete;
  ScopeGuard& operator=(ScopeGuard&&) = delete;

 private:
  F mFn;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_UTIL_SCOPEGUARD_HPP_
