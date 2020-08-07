/*
  MIT License

  Copyright (c) 2020 Sung Gon Kim

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

#ifndef INCLUDE_SENTINEL_UTIL_STRING_HPP_
#define INCLUDE_SENTINEL_UTIL_STRING_HPP_

#include <cctype>
#include <cstddef>
#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>


namespace sentinel {
namespace util {
namespace string {

/**
 * @brief Check if a string starts with the given keyword
 *
 * @param haystack base string
 * @param needle keyword
 * @return true if the string starts with the keyword, false otherwise
 */
inline bool startsWith(const std::string& haystack, const std::string& needle) {
  return (haystack.size() >= needle.size()) &&
      std::equal(needle.begin(), needle.end(), haystack.begin());
}

/**
 * @brief Check if a string ends with the given keyword
 *
 * @param haystack base string
 * @param needle keyword
 * @return true if the string ends with the keyword, false otherwise
 */
inline bool endsWith(const std::string& haystack, const std::string& needle) {
  return (haystack.size() >= needle.size()) &&
      std::equal(needle.rbegin(), needle.rend(), haystack.rbegin());
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
inline bool contains(const std::string& haystack,
                     const std::string& needle) {
  return haystack.find(needle) != std::string::npos;
}

/**
 * @brief Split a string with the given delimiter
 *
 * @param s string
 * @param delim delimiter
 * @return list of splitted strings
 */
inline std::vector<std::string> split(const std::string& s,
                                      const char delim = ' ') {
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
 * @brief Join the given characters into a string
 *
 * @param delim delimiter
 * @param tokens to join
 * @return joined string
 */
inline std::string join(const std::string& delim,
                        const std::vector<std::string>& tokens) {
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
inline std::string join(const char delim,
                        const std::vector<std::string>& tokens) {
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
inline std::string join(const std::string& delim,
                        const Arg&... tokens) {
  return join(delim, { tokens ... });
}

/**
 * @brief Join the given characters into a string
 *
 * @param delim delimter
 * @param tokens to join
 * @return joined string
 */
template <typename ... Arg>
inline std::string join(const char delim,
                        const Arg&... tokens) {
  return join(std::string(1, delim), { tokens ... });
}

}  // namespace string
}  // namespace util
}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_UTIL_STRING_HPP_
