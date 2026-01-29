/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_EXCEPTIONS_IOEXCEPTION_HPP_
#define INCLUDE_SENTINEL_EXCEPTIONS_IOEXCEPTION_HPP_

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

namespace sentinel {

/**
 * @brief IOException class
 */
class IOException : public std::runtime_error {
 public:
  /**
   * @brief Default constructor
   *
   * @param error code
   */
  explicit IOException(int error) :
      IOException(error, std::strerror(error)) {
  }

  /**
   * @brief Default constructor
   *
   * @param error code
   * @param message of the error
   */
  IOException(int error, const std::string& message) :
      std::runtime_error(message.c_str()), mError(error) {
  }

  /**
   * @brief Return the error code
   *
   * @return errno
   */
  int error() const {
    return -mError;
  }

 private:
  int mError;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_EXCEPTIONS_IOEXCEPTION_HPP_
