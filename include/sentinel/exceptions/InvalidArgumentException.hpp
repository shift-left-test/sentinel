/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_EXCEPTIONS_INVALIDARGUMENTEXCEPTION_HPP_
#define INCLUDE_SENTINEL_EXCEPTIONS_INVALIDARGUMENTEXCEPTION_HPP_

#include <stdexcept>
#include <string>

namespace sentinel {

/**
 * @brief InvalidArgumentException class
 */
class InvalidArgumentException : public std::invalid_argument {
 public:
  /**
   * @brief Default constructor
   *
   * @param message of the exception
   */
  explicit InvalidArgumentException(const std::string& message) :
      std::invalid_argument(message) {
  }
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_EXCEPTIONS_INVALIDARGUMENTEXCEPTION_HPP_
