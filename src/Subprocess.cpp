/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <sys/wait.h>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <exception>
#include <filesystem>  // NOLINT
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "sentinel/Console.hpp"
#include "sentinel/Subprocess.hpp"
#include "sentinel/util/signal.hpp"

namespace sentinel {

namespace fs = std::filesystem;

volatile pid_t Subprocess::childPid;
volatile std::size_t Subprocess::killAfter;
volatile bool Subprocess::timedOut;
volatile int Subprocess::pendSig;

Subprocess::Subprocess(const std::string& cmd, std::size_t sec, std::size_t secForKill,
                       const std::filesystem::path& logFile, bool silent) :
    mCmd(cmd), mSec(sec), mSecForKill(secForKill), mLogFile(logFile), mSilent(silent) {
  if (Subprocess::childPid != 0) {
    throw std::runtime_error("Another subprocess is running. (Problem with sentinel logic)");
  }
}

int Subprocess::execute() {
  // Check for existence of /bin/sh
  if (access("/bin/sh", X_OK) != 0) {
    throw std::runtime_error("/bin/sh is not an executable file.");
  }

  if (mCmd.empty()) {
    return -1;
  }

  // Open pipe
  int pfd[2];
  if (pipe(static_cast<int*>(pfd)) != 0) {
    throw std::runtime_error(fmt::format("failed to open pipe (cause: {})", std::strerror(errno)));
  }

  // Backup below signals' handler temporarily
  // sc's destructor restore signals' handler.
  const std::vector<int> usingSignals = {SIGABRT, SIGINT,  SIGFPE, SIGILL,  SIGSEGV,
                                         SIGTERM, SIGQUIT, SIGHUP, SIGALRM, SIGCHLD};
  auto sc = std::make_unique<signal::SaContainer>(usingSignals);

  // Ignore below signals' temporarily
  // If below signals are received before the signal handler is set,
  // sentinel operates abnormally.
  signal::setMultipleSignalHandlers(usingSignals, SIG_IGN);

  auto pid = fork();

  if (pid == 0) {
    // Redirect stdout and stderr to pipe
    close(pfd[0]);
    dup2(pfd[1], STDOUT_FILENO);
    dup2(pfd[1], STDERR_FILENO);
    close(pfd[1]);

    // When timeout occurs during test cmd, Sentinel send SIGTERM
    // to child process' group members. For that, child process must be
    // process group leader.
    ::setpgid(0, 0);

    // New process inherits SIG_IGN, so we need to restore signal handler.
    signal::setMultipleSignalHandlers(usingSignals, SIG_DFL);

    execlp("/bin/sh", "sh", "-c", mCmd.c_str(), nullptr);
    Console::err("Failed to execute command: {}", std::strerror(errno));
    exit(1);
  } else if (pid > 0) {
    // Close unused pipe
    close(pfd[1]);

    // set global variable
    Subprocess::childPid = pid;
    Subprocess::timedOut = false;
    Subprocess::pendSig = 0;
    Subprocess::killAfter = mSecForKill;

    int status;

    // If sentinel catch below signals,
    // then send SIGKILL to child process group.
    // And send last signal to sentinel just before return this function
    signal::setMultipleSignalHandlers({SIGABRT, SIGINT, SIGFPE, SIGILL, SIGSEGV, SIGTERM, SIGQUIT, SIGHUP},
                                      [](int signum) {
                                        Console::err("\nStopping due to {}...", strsignal(signum));
                                        kill(-Subprocess::childPid, SIGKILL);
                                        Subprocess::pendSig = signum;
                                      });

    // Just catch SIGCHLD
    signal::setMultipleSignalHandlers({SIGCHLD}, [](int signum) {});

    // SIGALRM handler
    // NOTE: Console::err uses fmt::format which is not strictly async-signal-safe,
    // but this is acceptable for the critical SIGKILL escalation message.
    signal::setMultipleSignalHandlers({SIGALRM}, [](int signum) {
      int termSignal = SIGTERM;
      if (!Subprocess::timedOut) {
        Subprocess::timedOut = true;
        alarm(Subprocess::killAfter);
      } else {
        termSignal = SIGKILL;
        Console::err("Failed to terminate child process within {}s. Sending {} to child process group.",
                     Subprocess::killAfter, strsignal(termSignal));
      }
      kill(-Subprocess::childPid, termSignal);
    });

    // Alarm setting
    alarm(mSec);

    // Open log file for tee output (optional)
    std::ofstream logStream;
    if (!mLogFile.empty()) {
      logStream.open(mLogFile.string(), std::ios::out | std::ios::trunc);
    }

    const int MAXBUFSZ = 4096;
    char buffer[MAXBUFSZ];

    while (waitpid(pid, &status, WNOHANG) == 0) {
      auto nb = read(pfd[0], static_cast<char*>(buffer), MAXBUFSZ);
      if (nb > 0) {
        if (!mSilent) {
          std::cout << std::string(static_cast<char*>(buffer), nb);
        }
        if (logStream.is_open()) {
          logStream.write(static_cast<char*>(buffer), nb);
        }
      }
    }

    // Drain any data remaining in the pipe after the child exited
    {
      ssize_t nb = 0;
      while ((nb = read(pfd[0], static_cast<char*>(buffer), MAXBUFSZ)) > 0) {
        if (!mSilent) {
          std::cout << std::string(static_cast<char*>(buffer), nb);
        }
        if (logStream.is_open()) {
          logStream.write(static_cast<char*>(buffer), nb);
        }
      }
    }

    // Alarm cancel
    alarm(0);

    // Close pipe
    close(pfd[0]);

    mStatus = status;
    if (Subprocess::timedOut && WIFSIGNALED(status) && (WTERMSIG(status) == SIGKILL || WTERMSIG(status) == SIGTERM)) {
      mTimedOut = true;
    }

    auto tmpSig = Subprocess::pendSig;

    // reset global variable
    Subprocess::childPid = 0;
    Subprocess::timedOut = false;
    Subprocess::pendSig = 0;
    Subprocess::killAfter = 0;

    // restore signal handler
    sc.reset();

    // send pending signal to sentinel
    if (tmpSig != 0) {
      kill(getpid(), tmpSig);
    }

    return status;
  } else {
    sc.reset();
    close(pfd[0]);
    close(pfd[1]);
    throw std::runtime_error(fmt::format("failed to fork ({}) (cause: {})", mCmd, std::strerror(errno)));
  }
}

bool Subprocess::isTimedOut() const {
  return mTimedOut;
}

bool Subprocess::isSuccessfulExit() const {
  return WIFEXITED(mStatus) && (WEXITSTATUS(mStatus) == 0);
}

}  // namespace sentinel
