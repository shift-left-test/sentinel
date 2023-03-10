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
 * @brief change MutationOperator to Expansion form 
 *
 * @param mo MutationOperator
 * @return Expansion form of MutationOperator
 *
 */
inline std::string MutationOperatorToExpansion(const std::string& mo) {
  static const std::map<std::string, std::string> expansion = {
    {"AOR", "AOR(arithmetic operator replacement)"},
    {"BOR", "BOR(bitwise operator replacement)"},
    {"LCR", "LCR(logical connector replacement)"},
    {"ROR", "ROR(relational operator replacement)"},
    {"SDL", "SDL(statement deletion)"},
    {"SOR", "SOR(shift operator replacement)"},
    {"UOI", "UOI(unary operator insertion)"}};
  return expansion.at(mo);
}

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_OPERATORS_MUTATIONOPERATOREXPANSION_HPP_
