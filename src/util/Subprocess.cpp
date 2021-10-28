/*
  MIT License

  Copyright (c) 2020 LG Electronics, Inc.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <fmt/core.h>
#include <unistd.h>
#include <sys/wait.h>
#include <chrono>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "sentinel/util/signal.hpp"
#include "sentinel/util/Subprocess.hpp"


namespace sentinel {

pid_t Subprocess::childPid;
std::size_t Subprocess::killAfter;
bool Subprocess::timedOut;
int Subprocess::pendSig;

Subprocess::Subprocess(const std::string& cmd, std::size_t sec,
    std::size_t secForKill) :
  mCmd(cmd), mSec(sec), mSecForKill(secForKill) {
    if (Subprocess::childPid != 0) {
      throw std::runtime_error(
          "Another subprocess is running. (Problem with sentinel logic)");
    }
}

int Subprocess::execute() {
  // Check for existence of /bin/sh
  if (access("/bin/sh", X_OK) != 0) {
    throw std::runtime_error("/bin/sh is not excutable file.");
  }

  if (mCmd.empty()) {
    return -1;
  }

  // Open pipe
  int pfd[2];
  if (pipe(static_cast<int*>(pfd)) != 0) {
    throw std::runtime_error(fmt::format("fail open pipe (cause: {})",
          std::strerror(errno)));
  }

  // Backup below signals' handler temporally
  // sc's desctructor restore signals' handler.
  const std::vector<int> usingSignals = {SIGABRT, SIGINT, SIGFPE, SIGILL,
    SIGSEGV, SIGTERM, SIGQUIT, SIGHUP, SIGALRM, SIGCHLD};
  auto sc = new signal::SaContainer(usingSignals);

  // Ignore below signals' temporally
  // If below signals are received before the signal handler is set,
  // sentinel operates abnormally.
  signal::setMultipleSignalHandlers(usingSignals, SIG_IGN);  // NOLINT

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
    std::cout << fmt::format("fail exec {} (cause: {})",
          mCmd, std::strerror(errno)) << std::endl;
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
    signal::setMultipleSignalHandlers({SIGABRT, SIGINT, SIGFPE, SIGILL,
        SIGSEGV, SIGTERM, SIGQUIT, SIGHUP},
        [](int signum) {
          std::cout << fmt::format(
              R"asdf(Receive a signal({}). Send a signal({}) to child process' process group.)asdf",
              strsignal(signum), strsignal(SIGKILL)) << std::endl;
          kill(-Subprocess::childPid, SIGKILL);
          Subprocess::pendSig = signum;
        });

    // Just catch SIGCHLD
    signal::setMultipleSignalHandlers({SIGCHLD}, [](int signum){});

    // SIGALRM handler
    signal::setMultipleSignalHandlers({SIGALRM},
        [](int signum) {
          int termSignal = SIGTERM;
          std::string tmpMsg;
          if (!Subprocess::timedOut) {
            Subprocess::timedOut = true;
            tmpMsg = "Timeout when executing test command.";
            alarm(Subprocess::killAfter);
          } else {
            termSignal = SIGKILL;
            tmpMsg = fmt::format("Failed to terminate child process within {}.",
                Subprocess::killAfter);
          }
          kill(-Subprocess::childPid, termSignal);
          std::cout << fmt::format(
              "{} Send a signal({}) to child process' process group.",
              tmpMsg, strsignal(termSignal)) << std::endl;
        });

    // Alarm setting
    alarm(mSec);

    const int MAXBUFSZ = 100;
    char buffer[MAXBUFSZ];

    while (waitpid(pid, &status, WNOHANG) == 0) {
      auto nb = read(pfd[0], static_cast<char*>(buffer), MAXBUFSZ);
      if (nb > 0) {
        std::cout << std::string(static_cast<char*>(buffer), nb);
      }
    }

    // Alarm cancel
    alarm(0);

    // Close pipe
    close(pfd[0]);

    mStatus = status;
    if (Subprocess::timedOut && WIFSIGNALED(status) &&
        (WTERMSIG(status) == SIGKILL || WTERMSIG(status) == SIGTERM))  {
      mTimedOut = true;
    }

    auto tmpSig = Subprocess::pendSig;

    // reset global variable
    Subprocess::childPid = 0;
    Subprocess::timedOut = false;
    Subprocess::pendSig = 0;
    Subprocess::killAfter = 0;

    // restore signal handler
    delete sc;

    // send pending signal to sentinel
    if (tmpSig != 0) {
      kill(getpid(), tmpSig);
    }

    return status;
  } else {
    delete sc;
    close(pfd[0]);
    close(pfd[1]);
    throw std::runtime_error(fmt::format("fail fork ({}) (cause: {})",
          mCmd, std::strerror(errno)));
  }
}

bool Subprocess::isTimedOut() {
  return mTimedOut;
}

bool Subprocess::isSuccessfulExit() {
  return WIFEXITED(mStatus) && (WEXITSTATUS(mStatus) == 0);
}

}  // namespace sentinel
