/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTATIONSTATE_HPP_
#define INCLUDE_SENTINEL_MUTATIONSTATE_HPP_

#include "sentinel/util/Utf8Char.hpp"

namespace sentinel {
/**
 * @brief Result State enumeration
 */
enum class MutationState : int { KILLED = 0, SURVIVED = 1, RUNTIME_ERROR = 2, BUILD_FAILURE = 3, TIMEOUT = 4 };

/**
 * @brief change MutationState to String
 *
 * @param m MutationState
 * @return meaning of MutationState
 *
 */
inline const char* MutationStateToStr(MutationState m) {
  switch (m) {
    case MutationState::KILLED:
      return "KILLED";
    case MutationState::SURVIVED:
      return "SURVIVED";
    case MutationState::RUNTIME_ERROR:
      return "RUNTIME_ERROR";
    case MutationState::BUILD_FAILURE:
      return "BUILD_FAILURE";
    case MutationState::TIMEOUT:
      return "TIMEOUT";
    default:
      return "UNKNOWN";
  }
}

/**
 * @brief Return the UTF-8 icon for a MutationState
 *
 * @param m MutationState
 * @return Utf8Char icon representing the state
 */
inline Utf8Char MutationStateIcon(MutationState m) {
  switch (m) {
    case MutationState::KILLED:
      return Utf8Char::CrossMark;
    case MutationState::SURVIVED:
      return Utf8Char::CheckMark;
    default:
      return Utf8Char::Warning;
  }
}

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONSTATE_HPP_
