/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTATIONSTATE_HPP_
#define INCLUDE_SENTINEL_MUTATIONSTATE_HPP_


namespace sentinel {
/**
 * @brief Result State enumeration
 */
enum class MutationState : int {
  KILLED = 0,
  SURVIVED = 1,
  RUNTIME_ERROR = 2,
  BUILD_FAILURE = 3,
  TIMEOUT = 4
};

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
      return "UNKOWN";
  }
}

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONSTATE_HPP_
