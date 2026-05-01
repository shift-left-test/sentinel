/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <llvm/Support/ErrorHandling.h>
#include <unistd.h>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <new>
#include "sentinel/OomHandler.hpp"

namespace sentinel {

namespace {

// First-writer wins across threads. Sibling threads must stop allocating;
// otherwise glibc reports "free(): corrupted unsorted chunks" because
// LLVM's report_fatal_error path races with malloc in those threads.
std::atomic_flag sHandled = ATOMIC_FLAG_INIT;

// Same-thread reentrancy guard. SignalHandler::dispatch runs cleanup
// callbacks synchronously after raise(SIGUSR1); if any callback hits
// another bad_alloc on this same thread, the recursive handler call must
// terminate immediately rather than parking — otherwise the thread hangs
// in pause() and never reaches _exit.
thread_local bool sInHandler = false;

constexpr int kOomExitCode = 137;            // 128 + SIGKILL, OOM-killed convention
constexpr const char kPrefix[] = "ERROR: ";  // matches Logger::error prefix
constexpr const char kBadAllocMessage[] = "out of memory during mutation generation\n";

[[noreturn]] void parkForever() noexcept {
  for (;;) {
    ::pause();
  }
}

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
  // without exiting; _exit then terminates before other worker threads can
  // continue allocating and before C++ static destructors race.
  std::raise(SIGUSR1);
  // cppcheck-suppress unreachableCode
  ::_exit(kOomExitCode);
}

void enterHandlerOrTerminate() noexcept {
  if (sInHandler) {
    ::_exit(kOomExitCode);
  }
  sInHandler = true;
  if (sHandled.test_and_set(std::memory_order_acq_rel)) {
    parkForever();
  }
}

extern "C" void onLlvmBadAlloc(void* /*userData*/, const char* /*reason*/, bool /*genCrashDiag*/) {
  enterHandlerOrTerminate();
  writeRaw(kPrefix, sizeof(kPrefix) - 1);
  writeRaw(kBadAllocMessage, sizeof(kBadAllocMessage) - 1);
  runCleanupAndExit();
}

extern "C" void onLlvmFatal(void* /*userData*/, const char* reason, bool /*genCrashDiag*/) {
  enterHandlerOrTerminate();
  writeRaw(kPrefix, sizeof(kPrefix) - 1);
  if (reason != nullptr) {
    writeRaw(reason, std::strlen(reason));
  }
  writeRaw("\n", 1);
  runCleanupAndExit();
}

}  // namespace

void installOomHandlers() {
  llvm::install_bad_alloc_error_handler(onLlvmBadAlloc);
  llvm::install_fatal_error_handler(onLlvmFatal);
  std::set_new_handler([] { onLlvmBadAlloc(nullptr, "std::new failed", false); });
}

}  // namespace sentinel
