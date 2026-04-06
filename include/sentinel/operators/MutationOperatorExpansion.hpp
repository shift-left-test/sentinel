/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_OPERATORS_MUTATIONOPERATOREXPANSION_HPP_
#define INCLUDE_SENTINEL_OPERATORS_MUTATIONOPERATOREXPANSION_HPP_

#include <map>
#include <string>
#include <vector>

namespace sentinel {

namespace detail {

/**
 * @brief Return the internal map of operator abbreviations to descriptions.
 */
inline const std::map<std::string, std::string>& operatorExpansionMap() {
  static const std::map<std::string, std::string> expansion = {
      {"AOR", "AOR (Arithmetic Operator Replacement)"},
      {"BOR", "BOR (Bitwise Operator Replacement)"},
      {"LCR", "LCR (Logical Connector Replacement)"},
      {"ROR", "ROR (Relational Operator Replacement)"},
      {"SDL", "SDL (Statement Deletion)"},
      {"SOR", "SOR (Shift Operator Replacement)"},
      {"UOI", "UOI (Unary Operator Insertion)"}};
  return expansion;
}

}  // namespace detail

/**
 * @brief Check whether the given name is a valid mutation operator
 *        (case-sensitive, uppercase expected).
 * @param name operator name to check
 * @return true if valid
 */
inline bool isValidOperator(const std::string& name) {
  return detail::operatorExpansionMap().count(name) > 0;
}

/**
 * @brief Return the list of valid mutation operator names.
 * @return const reference to the sorted name list
 */
inline const std::vector<std::string>& validOperatorNames() {
  static const auto names = [] {
    const auto& map = detail::operatorExpansionMap();
    std::vector<std::string> v;
    v.reserve(map.size());
    for (const auto& [key, _] : map) {
      v.push_back(key);
    }
    return v;
  }();
  return names;
}

/**
 * @brief Convert a mutation operator abbreviation to its expanded form.
 * @param mo operator abbreviation (e.g. "AOR")
 * @return expanded description (e.g. "AOR (Arithmetic Operator Replacement)")
 * @throw std::out_of_range if mo is not a valid operator
 */
inline std::string MutationOperatorToExpansion(const std::string& mo) {
  return detail::operatorExpansionMap().at(mo);
}

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_OPERATORS_MUTATIONOPERATOREXPANSION_HPP_
