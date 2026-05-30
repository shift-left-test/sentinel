/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <llvm/Config/llvm-config.h>
#include <llvm/Support/ErrorHandling.h>
#include <unistd.h>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include "sentinel/OomHandler.hpp"

namespace sentinel {

namespace {

// LLVM 14 changed fatal_error_handler_t's reason parameter from
// `const std::string&` to `const char*`. Alias the type so the handler
// signatures match whichever LLVM the build links against.
#if LLVM_VERSION_MAJOR >= 14
using LlvmErrorReason = const char*;
#else
using LlvmErrorReason = const std::string&;
#endif

// Re-entry guard. SignalHandler::dispatch runs cleanup callbacks
// synchronously after raise(SIGUSR1); if any callback triggers another
// bad_alloc, the recursive handler call must terminate immediately
// rather than re-running cleanup.
bool sInHandler = false;

constexpr int kOomExitCode = 137;            // 128 + SIGKILL, OOM-killed convention
constexpr const char kPrefix[] = "ERROR: ";  // matches Logger::error prefix
constexpr const char kBadAllocMessage[] = "out of memory during mutation generation\n";

void writeRaw(const char* buf, std::size_t len) noexcept {
  while (len > 0) {
    ssize_t n = ::write(STDERR_FILENO, buf, len);
    if (n > 0) {
      buf += n;
      len -= static_cast<std::size_t>(n);
      continue;
    }
    if (n == -1 && errno == EINTR) {
      continue;
    }
    break;
  }
}

[[noreturn]] void runCleanupAndExit() noexcept {
  // SIGUSR1 runs SignalHandler cleanup (backup restore, StatusLine disable)
  // without exiting; _exit then terminates before C++ static destructors
  // can run on a process whose heap is exhausted.
  std::raise(SIGUSR1);
  // cppcheck-suppress unreachableCode
  ::_exit(kOomExitCode);
}

// Reason-free entry point. Kept separate so set_new_handler can report OOM
// without constructing a std::string temporary (which would itself allocate
// on the LLVM < 14 `const std::string&` path).
[[noreturn]] void reportBadAllocAndExit() noexcept {
  if (sInHandler) {
    ::_exit(kOomExitCode);
  }
  sInHandler = true;
  writeRaw(kPrefix, sizeof(kPrefix) - 1);
  writeRaw(kBadAllocMessage, sizeof(kBadAllocMessage) - 1);
  runCleanupAndExit();
}

extern "C" void onLlvmBadAlloc(void* /*userData*/, LlvmErrorReason /*reason*/, bool /*genCrashDiag*/) {
  reportBadAllocAndExit();
}

extern "C" void onLlvmFatal(void* /*userData*/, LlvmErrorReason reason, bool /*genCrashDiag*/) {
  if (sInHandler) {
    ::_exit(kOomExitCode);
  }
  sInHandler = true;
  writeRaw(kPrefix, sizeof(kPrefix) - 1);
#if LLVM_VERSION_MAJOR >= 14
  if (reason != nullptr) {
    writeRaw(reason, std::strlen(reason));
  }
#else
  writeRaw(reason.data(), reason.size());
#endif
  writeRaw("\n", 1);
  runCleanupAndExit();
}

}  // namespace

void installOomHandlers() {
  llvm::install_bad_alloc_error_handler(onLlvmBadAlloc);
  llvm::install_fatal_error_handler(onLlvmFatal);
  std::set_new_handler([] { reportBadAllocAndExit(); });
}

}  // namespace sentinel
