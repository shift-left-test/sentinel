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
    default:
      return "UNKOWN";
  }
}

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONSTATE_HPP_
