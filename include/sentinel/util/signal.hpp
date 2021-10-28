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

#ifndef INCLUDE_SENTINEL_UTIL_SIGNAL_HPP_
#define INCLUDE_SENTINEL_UTIL_SIGNAL_HPP_

#include <csignal>
#include <algorithm>
#include <string>
#include <tuple>
#include <vector>


namespace sentinel {
namespace signal {

/**
 * @brief set signal handler
 *
 * @param signum signal name
 * @param handler signal handler
 */
inline void setSignalHandler(int signum, void (*handler)(int)) {
  struct sigaction target{};
  target.sa_handler = handler;  // NOLINT
  sigemptyset(&target.sa_mask);
  target.sa_flags = 0;
  ::sigaction(signum, &target, nullptr);
}

/**
 * @brief set multiple signal handlers
 *
 * @param setSignum Set of signal names
 * @param handler signal handler
 */
inline void setMultipleSignalHandlers(const std::vector<int>& setSignum,
    void (*handler)(int)) {
  for (auto target : setSignum) {
    setSignalHandler(target, handler);
  }
}

/**
 * @brief get sigaction
 *
 * @param signum signal name
 * @param [out] current sigaction
 */
inline void getSigaction(int signum, struct sigaction* current) {
    ::sigaction(signum, nullptr, current);
}

/**
 * @brief set sigaction
 *
 * @param signum signal name
 * @param newSigaction
 */
inline void setSigaction(int signum, struct sigaction* newSigaction) {
    ::sigaction(signum, newSigaction, nullptr);
}

/**
 * @brief Sigaction Container
 */
class SaContainer {
 public:
  /**
   * @brief constructor
   *
   * @param signums target signums
   */
  explicit SaContainer(const std::vector<int>& signums) {
    std::transform(std::begin(signums), std::end(signums),
        std::back_inserter(signumAndSa),
        [](int signum) -> std::tuple<int, struct sigaction*> {
          struct sigaction* sa = new struct sigaction;
          getSigaction(signum, sa);
          return std::make_tuple(signum, sa);
        });
  }

  /**
   * @brief destructor
   */
  ~SaContainer() {
    std::for_each(std::begin(signumAndSa), std::end(signumAndSa),
        [](const std::tuple<int, struct sigaction*>& current) {
          setSigaction(std::get<0>(current), std::get<1>(current));
          delete std::get<1>(current);
        });
  }

 private:
  std::vector<std::tuple<int, struct sigaction*>> signumAndSa;
};

}  // namespace signal
}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_UTIL_SIGNAL_HPP_
