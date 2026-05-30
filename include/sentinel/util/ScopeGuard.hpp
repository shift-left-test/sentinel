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
  /// The guard often runs during stack unwinding (cleanup on a thrown path).
  /// Destructors are implicitly noexcept, so any exception escaping mFn() would
  /// call std::terminate. Swallow it: scope-guard cleanup is best-effort and the
  /// callable is expected to surface failures itself (e.g. via logging).
  ~ScopeGuard() {
    try {
      mFn();
    } catch (...) {  // NOLINT(bugprone-empty-catch)
    }
  }
  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard& operator=(const ScopeGuard&) = delete;
  ScopeGuard(ScopeGuard&&) = delete;
  ScopeGuard& operator=(ScopeGuard&&) = delete;

 private:
  F mFn;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_UTIL_SCOPEGUARD_HPP_
