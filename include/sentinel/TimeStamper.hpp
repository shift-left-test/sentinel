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
 * @brief TimeStamper class.
 */
class TimeStamper {
 public:
  /**
   * @brief Format enumeration.
   */
  enum class Format {
    Clock,
    HumanReadable
  };

  /**
   * @brief Default constructor.
   */
  TimeStamper();
  TimeStamper(const TimeStamper&) = delete;
  TimeStamper& operator=(const TimeStamper&) = delete;

  /**
   * @brief Default destructor.
   */
  ~TimeStamper();

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
