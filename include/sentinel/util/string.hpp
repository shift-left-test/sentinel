/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_UTIL_STRING_HPP_
#define INCLUDE_SENTINEL_UTIL_STRING_HPP_

#include <fmt/core.h>
#include <fmt/format.h>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "sentinel/exceptions/InvalidArgumentException.hpp"

namespace sentinel {

namespace string {

/**
 * @brief Check if a string starts with the given keyword.
 *
 * @param haystack base string
 * @param needle keyword
 * @return true if the string starts with the keyword, false otherwise
 */
inline bool startsWith(const std::string& haystack, const std::string& needle) {
  return (haystack.size() >= needle.size()) && std::equal(needle.begin(), needle.end(), haystack.begin());
}

/**
 * @brief Check if a string ends with the given keyword
 *
 * @param haystack base string
 * @param needle keyword
 * @return true if the string ends with the keyword, false otherwise
 */
inline bool endsWith(const std::string& haystack, const std::string& needle) {
  return (haystack.size() >= needle.size()) && std::equal(needle.rbegin(), needle.rend(), haystack.rbegin());
}

/**
 * @brief Remove leading characters based on the given predicate
 *
 * @param s string
 * @param pred predicate
 * @return trimmed characters
 */
template <typename UnaryPredicate>
inline std::string ltrim(const std::string& s, UnaryPredicate pred) {
  auto first = std::find_if_not(s.begin(), s.end(), pred);
  auto last = s.end();
  return std::string(first, last);
}

/**
 * @brief Remove leading whitespace characters
 *
 * @param s string
 * @return trimmed characteres
 */
inline std::string ltrim(const std::string& s) {
  return ltrim(s, [](unsigned char c) { return std::isspace(c); });
}

/**
 * @brief Remove trailing characters based on the given predicate
 *
 * @param s string
 * @param pred predicate
 * @return trimmed characters
 */
template <typename UnaryPredicate>
inline std::string rtrim(const std::string& s, UnaryPredicate pred) {
  auto first = s.begin();
  auto last = std::find_if_not(s.rbegin(), s.rend(), pred).base();
  return std::string(first, last);
}

/**
 * @brief Remove trailing whitespace characters
 *
 * @param s string
 * @return trimmed characters
 */
inline std::string rtrim(const std::string& s) {
  return rtrim(s, [](unsigned char c) { return std::isspace(c); });
}

/**
 * @brief Remove leading and trailing characters based on the given predicate
 *
 * @param s string
 * @param pred predicate
 * @return trimmed characters
 */
template <typename UnaryPredicate>
inline std::string trim(const std::string& s, UnaryPredicate pred) {
  return ltrim(rtrim(s, pred), pred);
}

/**
 * @brief Remove leading and trailing whitespace characters
 *
 * @param s string
 * @return trimmed characters
 */
inline std::string trim(const std::string& s) {
  return trim(s, [](unsigned char c) { return std::isspace(c); });
}

/**
 * @brief Check if the given keyword exists within the string
 *
 * @param haystack string
 * @param needle keyword
 * @return true if the keyword exists, false otherwise
 */
inline bool contains(const std::string& haystack, const std::string& needle) {
  return haystack.find(needle) != std::string::npos;
}

/**
 * @brief Split a string with the given delimiter
 *
 * @param s string
 * @param delim delimiter
 * @return list of splitted strings
 */
inline std::vector<std::string> split(const std::string& s, char delim = ' ') {
  std::vector<std::string> tokens;
  std::istringstream ss;
  ss.str(s);
  std::string token;
  while (std::getline(ss, token, delim)) {
    tokens.emplace_back(token);
  }
  return tokens;
}

/**
 * @brief Return vector of string that is split by string delimiter
 *
 * @param s target string
 * @param delimiter
 * @return vecotr of string that is split by delimiter
 */
inline std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
  std::size_t pos_start = 0;
  std::size_t pos_end;
  std::size_t delim_len = delimiter.length();
  std::string token;
  std::vector<std::string> res;
  while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
    token = s.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + delim_len;
    res.push_back(token);
  }
  res.push_back(s.substr(pos_start));
  return res;
}

/**
 * @brief Join the given characters into a string
 *
 * @param delim delimiter
 * @param tokens to join
 * @return joined string
 */
inline std::string join(const std::string& delim, const std::vector<std::string>& tokens) {
  std::ostringstream oss;
  auto first = std::begin(tokens);
  auto last = std::end(tokens);
  while (first != last) {
    oss << *first;
    if (first != std::prev(last)) {
      oss << delim;
    }
    ++first;
  }
  return oss.str();
}

/**
 * @brief Join the given characters into a string
 *
 * @param delim delimiter
 * @param tokens to join
 * @return joined string
 */
inline std::string join(char delim, const std::vector<std::string>& tokens) {
  return join(std::string(1, delim), tokens);
}

/**
 * @brief Join the given characters into a string
 *
 * @param delim delimter
 * @param tokens to join
 * @return joined string
 */
template <typename ... Arg>
inline std::string join(const std::string& delim, const Arg&... tokens) {
  return join(delim, { tokens ... });  // NOLINT
}

/**
 * @brief Join the given characters into a string
 *
 * @param delim delimter
 * @param tokens to join
 * @return joined string
 */
template <typename ... Arg>
inline std::string join(char delim, const Arg&... tokens) {
  return join(std::string(1, delim), { tokens ... });
}

/**
 * @brief replace all 'oldStr' in 'target' with 'newStr'
 *
 * @param target string
 * @param oldStr
 * @param newStr
 * @return joined string
 */
inline std::string replaceAll(std::string target, const std::string& oldStr, const std::string& newStr) {
  std::size_t pos = 0;
  if (oldStr.empty() || target.empty()) {
    return target;
  }
  while ((pos = target.find(oldStr, pos)) != std::string::npos) {
    target.replace(pos, oldStr.length(), newStr);
    pos += newStr.length();
  }
  return target;
}

/**
 * @brief convert boolean to string
 *
 * @param b boolean value
 * @return string value of b
 */
inline const char* boolToString(bool b) {
  return b ? "true" : "false";
}

/**
 * @brief convert string to integer
 *
 * @param target string
 * @return converted integer value
 */
template<typename T>
T stringToInt(const std::string& s) {
  std::string str = string::trim(s);
  if (str.at(0) == '+') {
    str = str.replace(0, 1, "");
  }
  T ret;
  std::stringstream ss(str);
  ss >> ret;
  if (std::to_string(ret) != str) {
    throw InvalidArgumentException("Can't convert " + str);
  }
  return ret;
}

/**
 * @brief convert string to boolean
 *
 * @param s target string
 * @return boolean value of string
 * @throw InvalidArgumentException
 */
inline bool stringToBool(const std::string& s) {
  std::string temp = s;
  if (temp == "true") {
    return true;
  }

  if (temp == "false") {
    return false;
  }

  throw InvalidArgumentException(
      fmt::format("Input must be a boolean value (true or false): \"{}\". temp = {}", s, temp));
}

}  // namespace string

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_UTIL_STRING_HPP_
