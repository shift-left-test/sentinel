/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_EXCEPTIONS_REPOSITORYEXCEPTION_HPP_
#define INCLUDE_SENTINEL_EXCEPTIONS_REPOSITORYEXCEPTION_HPP_

#include <stdexcept>
#include <string>


namespace sentinel {

/**
 * @brief RepositoryExcpetion class
 */
class RepositoryException : public std::runtime_error {
 public:
  /**
   * @brief Default constructor
   *
   * @param message of the error
   */
  explicit RepositoryException(const std::string& message) :
      std::runtime_error(message.c_str()) {
  }
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_EXCEPTIONS_REPOSITORYEXCEPTION_HPP_
