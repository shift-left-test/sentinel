/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_TIMESTAMPER_HPP_
#define INCLUDE_SENTINEL_TIMESTAMPER_HPP_

#include <chrono>
#include <string>

namespace sentinel {

/**
 * @brief Timestamper class.
 */
class Timestamper {
 public:
  /**
   * @brief Format enumeration.
   */
  enum class Format { Clock, HumanReadable };

  /**
   * @brief Default constructor.
   */
  Timestamper();
  Timestamper(const Timestamper&) = delete;
  Timestamper& operator=(const Timestamper&) = delete;

  /**
   * @brief Default destructor.
   */
  ~Timestamper();

  /**
   * @brief Reset timer.
   */
  void reset();

  /**
   * @brief Get elapsed time in seconds.
   *
   * @return seconds
   */
  double toDouble() const;

  /**
   * @brief Format an arbitrary duration in seconds.
   *
   * @param secs duration in seconds
   * @param style to format
   * @return formatted string
   */
  static std::string format(double secs, Format style = Format::HumanReadable);

  /**
   * @brief Get formatted elapsed time.
   *
   * @param style to format
   * @return formatted string
   */
  std::string toString(Format style = Format::Clock) const;

 private:
  std::chrono::steady_clock::time_point mStartTime;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_TIMESTAMPER_HPP_
