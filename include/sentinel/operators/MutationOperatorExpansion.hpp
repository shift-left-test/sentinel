/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_OPERATORS_MUTATIONOPERATOREXPANSION_HPP_
#define INCLUDE_SENTINEL_OPERATORS_MUTATIONOPERATOREXPANSION_HPP_

#include <map>
#include <string>

namespace sentinel {
/**
 * @brief Return the map of valid mutation operator abbreviations to their
 *        expanded descriptions.
 * @return const reference to the operator expansion map
 */
inline const std::map<std::string, std::string>& MutationOperatorExpansionMap() {
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

/**
 * @brief change MutationOperator to Expansion form
 *
 * @param mo MutationOperator
 * @return Expansion form of MutationOperator
 *
 */
inline std::string MutationOperatorToExpansion(const std::string& mo) {
  return MutationOperatorExpansionMap().at(mo);
}

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_OPERATORS_MUTATIONOPERATOREXPANSION_HPP_
