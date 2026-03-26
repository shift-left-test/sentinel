/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <chrono>
#include <cmath>
#include <string>
#include "sentinel/Timestamper.hpp"

namespace sentinel {

Timestamper::Timestamper() {
  reset();
}

Timestamper::~Timestamper() = default;

void Timestamper::reset() {
  mStartTime = std::chrono::steady_clock::now();
}

double Timestamper::toDouble() const {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration<double>(now - mStartTime).count();
}

std::string Timestamper::format(double secs, Format style) {
  int h = static_cast<int>(secs / 3600);
  int m = static_cast<int>(std::fmod(secs, 3600.0) / 60.0);
  double s = std::fmod(secs, 60.0);

  if (style == Format::Clock) {
    return fmt::format("{:02d}:{:02d}:{:02d}", h, m, static_cast<int>(s));
  }
  if (style == Format::HumanReadable) {
    if (h > 0) return fmt::format("{}h {}m", h, m);
    if (m > 0) return fmt::format("{}m {:.0f}s", m, s);
    return fmt::format("{:.0f}s", s);
  }
  return "Unknown format";
}

std::string Timestamper::toString(Format style) const {
  return format(toDouble(), style);
}

}  // namespace sentinel
