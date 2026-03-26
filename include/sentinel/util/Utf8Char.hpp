/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_UTIL_UTF8CHAR_HPP_
#define INCLUDE_SENTINEL_UTIL_UTF8CHAR_HPP_

#include <fmt/format.h>
#include <cstddef>
#include <cstring>
#include <string>

namespace sentinel {

/**
 * @brief A lightweight wrapper for a single UTF-8 encoded character.
 *
 * Provides named constants for commonly used Unicode symbols and
 * operator* for repeating the character N times.
 */
class Utf8Char {
  const char* mData;

 public:
  /**
   * @brief Constructs a Utf8Char from a null-terminated UTF-8 byte sequence.
   * @param data pointer to a null-terminated UTF-8 encoded character
   */
  constexpr explicit Utf8Char(const char* data) : mData(data) {}

  /**
   * @brief Returns the underlying null-terminated UTF-8 byte sequence.
   * @return pointer to the UTF-8 encoded character
   */
  const char* c_str() const { return mData; }

  /**
   * @brief Repeats this character n times.
   * @param n number of repetitions
   * @return a string containing the character repeated n times
   */
  std::string operator*(std::size_t n) const {
    std::size_t len = std::strlen(mData);
    std::string result;
    result.reserve(len * n);
    for (std::size_t i = 0; i < n; ++i) {
      result.append(mData, len);
    }
    return result;
  }

  static const Utf8Char CheckMark;    ///< ✓ (U+2713)
  static const Utf8Char CrossMark;    ///< ✗ (U+2717)
  static const Utf8Char Warning;      ///< ⚠ (U+26A0)
  static const Utf8Char ArrowRight;   ///< → (U+2192)
  static const Utf8Char ArrowLeft;    ///< ← (U+2190)
  static const Utf8Char ArrowHook;    ///< ↪ (U+21AA)
  static const Utf8Char EmDash;       ///< — (U+2014)
  static const Utf8Char ThickLine;    ///< ━ (U+2501)
  static const Utf8Char ThinLine;     ///< ─ (U+2500)
};

inline constexpr Utf8Char Utf8Char::CheckMark{"\xe2\x9c\x93"};
inline constexpr Utf8Char Utf8Char::CrossMark{"\xe2\x9c\x97"};
inline constexpr Utf8Char Utf8Char::Warning{"\xe2\x9a\xa0"};
inline constexpr Utf8Char Utf8Char::ArrowRight{"\xe2\x86\x92"};
inline constexpr Utf8Char Utf8Char::ArrowLeft{"\xe2\x86\x90"};
inline constexpr Utf8Char Utf8Char::ArrowHook{"\xe2\x86\xaa"};
inline constexpr Utf8Char Utf8Char::EmDash{"\xe2\x80\x94"};
inline constexpr Utf8Char Utf8Char::ThickLine{"\xe2\x94\x81"};
inline constexpr Utf8Char Utf8Char::ThinLine{"\xe2\x94\x80"};

}  // namespace sentinel

/// @cond INTERNAL
template <>
struct fmt::formatter<sentinel::Utf8Char> : fmt::formatter<const char*> {
  template <typename FormatContext>
  auto format(const sentinel::Utf8Char& ch, FormatContext& ctx) {  // NOLINT
    return fmt::formatter<const char*>::format(ch.c_str(), ctx);
  }
};
/// @endcond

#endif  // INCLUDE_SENTINEL_UTIL_UTF8CHAR_HPP_
