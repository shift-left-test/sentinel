/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_UTIL_FORMATTER_HPP_
#define INCLUDE_SENTINEL_UTIL_FORMATTER_HPP_

#include <fmt/format.h>
#include <filesystem>  // NOLINT
#include <string>

/// @cond INTERNAL
template <>
struct fmt::formatter<std::filesystem::path> : fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const std::filesystem::path& p, FormatContext& ctx) {  // NOLINT
    return fmt::formatter<std::string>::format(p.lexically_normal().string(), ctx);
  }
};
/// @endcond

#endif  // INCLUDE_SENTINEL_UTIL_FORMATTER_HPP_
