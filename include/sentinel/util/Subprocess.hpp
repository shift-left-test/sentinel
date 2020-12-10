/*
  MIT License

  Copyright (c) 2020 Sangmo Kang

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

#ifndef INCLUDE_SENTINEL_UTIL_SUBPROCESS_HPP_
#define INCLUDE_SENTINEL_UTIL_SUBPROCESS_HPP_

#include <unistd.h>
#include <sys/wait.h>
#include <chrono>
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include "sentinel/util/signal.hpp"


namespace sentinel {

/**
 * @brief Subprocess class
 */
class Subprocess {
 public:
  /**
   * @brief Default constructor
   *
   * @param cmd to excute
   * @param sec for timeout
   */
  explicit Subprocess(const std::string& cmd, std::size_t sec = 0);

  /**
   * @brief excute cmd
   *
   * @return exit status
   */
  int execute();

  /**
   * @brief check if timeout occurs
   *
   * @return 1 if timeout occurs
   */
  bool isTimedOut();

  /**
   * @brief check if exit and return 0
   *
   * @return 1 if exit and retun 0
   */
  bool isSuccessfulExit();

 private:
  std::string mCmd;
  std::size_t mSec;
  bool mTimedOut = false;
  int mStatus = -1;
  static void sigHandler(int signum);
};

Subprocess::Subprocess(const std::string& cmd, std::size_t sec) :
  mCmd(cmd), mSec(sec) {
}

int Subprocess::execute() {
  // When timeout occurs during test cmd, Sentinel send SIGTERM
  // to all process group member. It is possible that Sentinel is belong to
  // parent process' group. Then, sentinel will be try to kill parent process.
  // Avoding that situation, Sentinel must be proces group leader.
  ::setpgid(0, 0);
  auto pid = fork();

  if (access("/bin/sh", X_OK) != 0) {
    throw std::runtime_error("/bin/sh is not excutable file.");
  }
  if (mCmd.empty()) {
    return -1;
  }

  if (pid == 0) {
    execlp("/bin/sh", "sh", "-c", mCmd.c_str(), nullptr);
    std::cout << fmt::format("fail exec {} (cause: {})",
          mCmd, std::strerror(errno)) << std::endl;
    exit(1);
  } else if (pid > 0) {
    int status;
    // Backup below signals' handler temporally
    // When going out this block, sc's desctructor restore signals' handler.
    signal::SaContainer sc(
        {SIGINT, SIGQUIT, SIGALRM, SIGTERM, SIGCHLD, SIGKILL, SIGSTOP});

    // SIGINT and SIGQUIT affect only child process during waitpid.
    // SIGTERM is sent whole current process group to kill child process,
    // so we need to ignore SIGTERM signal during waitpid.
    signal::setMultipleSignalHandlers(
        {SIGINT, SIGQUIT, SIGTERM}, SIG_IGN);  // NOLINT

    // If SIGALRM, SIGKILL, SIGSTOP are caught, kill child process
    // SIGKILL and SIGSTOP(unblocked signal) -> avoiding zombie process
    signal::setMultipleSignalHandlers(
        {SIGALRM, SIGKILL, SIGSTOP}, sigHandler);

    // Temporally Block whole signal except SIGCHLD and SIGALRM during waitpid.
    signal::setSignalHandler(SIGCHLD, [](int signum){});
    sigset_t block;
    sigfillset(&block);
    sigdelset(&block, SIGALRM);
    sigdelset(&block, SIGCHLD);

    // Alarm setting
    alarm(mSec);
    while (waitpid(pid, &status, WNOHANG) == 0) {
      // Suspend until SIGALRM or SIGCHLD is sent
      sigsuspend(&block);
    }
    // Alarm cancel. If alarm already rang(timeout occured), return 0.
    auto remain = alarm(0);
    mTimedOut = (remain == 0);
    mStatus = status;
    return status;
  } else {
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

void Subprocess::sigHandler(int signum) {
  kill(0, SIGTERM);
  if (signum == SIGALRM) {
    std::cout << "Timeout when executing test command." << std::endl;
  }
}

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_UTIL_SUBPROCESS_HPP_
