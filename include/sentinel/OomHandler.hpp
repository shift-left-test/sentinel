/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_OOMHANDLER_HPP_
#define INCLUDE_SENTINEL_OOMHANDLER_HPP_

namespace sentinel {

/**
 * @brief Install handlers that turn LLVM/Clang OOM and fatal errors into a
 *        deterministic exit with a fixed ERROR message.
 *
 * After this call:
 *   - llvm::install_bad_alloc_error_handler intercepts LLVM-detected OOM.
 *   - llvm::install_fatal_error_handler intercepts other unrecoverable
 *     LLVM errors (e.g. "Cannot chdir into ...").
 *   - std::set_new_handler covers C++ `new` failures outside LLVM.
 *
 * The handlers do not allocate, format, or call Logger; they emit a
 * fixed stderr message via write(2), raise SIGUSR1 so SignalHandler can
 * run cleanup callbacks (backup restore, StatusLine disable), and then
 * call _exit(137). Exit code 137 follows the OOM-killed convention
 * (128 + 9).
 *
 * Must be called after sentinel::SignalHandler::add() has registered the
 * cleanup callbacks for SIGUSR1, otherwise the SIGUSR1 raise will not
 * trigger backup restoration.
 */
void installOomHandlers();

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_OOMHANDLER_HPP_
