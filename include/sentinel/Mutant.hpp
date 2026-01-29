/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTANT_HPP_
#define INCLUDE_SENTINEL_MUTANT_HPP_

#include <experimental/filesystem>
#include <iostream>
#include <string>
#include <utility>
#include "sentinel/SourceTree.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

/**
 * @brief Location struct
 */
struct Location {
  /**
   * @brief Default constructor
   *
   * @param lineNum number
   * @param colNum number
   */
  Location(std::size_t lineNum, std::size_t colNum) :
      line(lineNum), column(colNum) {
  }

  /**
   * @brief line number
   */
  std::size_t line;

  /**
   * @brief column number
   */
  std::size_t column;
};

class SourceTree;

/**
 * @brief Mutant class
 */
class Mutant {
 public:
  /**
   * @brief Default constructor
   */
  Mutant();

  /**
   * @brief Default constructor
   *
   * @param mutationOperator created this mutable
   * @param path to file from the source tree
   * @param qualifiedFuncName name of function containing mutation location
   * @param firstLine line number of first location
   * @param firstColumn column number of first location
   * @param lastLine line number of last location
   * @param lastColumn column number of last location
   * @param token to replace with
   */
  Mutant(const std::string& mutationOperator,
         const std::string& path,
         const std::string& qualifiedFuncName,
         std::size_t firstLine,
         std::size_t firstColumn,
         std::size_t lastLine,
         std::size_t lastColumn,
         const std::string& token);

  /**
   * @brief == operator overloading
   *
   * @param other Mutant
   * @return True if Mutants are same. False otherwise
   */
  bool operator==(const Mutant& other) const;

  /**
   * @brief != operator overloading
   *
   * @param other Mutant
   * @return True if Mutants are different. False otherwise
   */
  bool operator!=(const Mutant& other) const;

  /**
   * @brief < operator overloading
   *
   * @param other Mutant
   * @return True if this Mutant is less than other Mutant. False otherwise
   */
  bool operator<(const Mutant& other) const;

  /**
   * @brief Return the mutation operator creating this mutable.
   *
   * @return mutation operator
   */
  std::string getOperator() const;

  /**
   * @brief Return the path of mutated file.
   *
   * @return path to the file
   */
  std::experimental::filesystem::path getPath() const;

  /**
   * @brief Return the namespace, class containing mutation location.
   *        Return empty string if those do not exist.
   *
   * @return namespace, class containing mutation location
   */
  std::string getClass() const;

  /**
   * @brief Return the name of function containing mutation location.
   *        Return empty string if such function does not exist.
   *
   * @return function containing mutation location
   */
  std::string getFunction() const;

   /**
   * @brief Return the namespace, class, function containing mutation location.
   *        Return empty string if such function does not exist.
   *
   * @return namespace, class, function containing mutation location
   */
  std::string getQualifiedFunction() const;

  /**
   * @brief Return the first location to apply mutation.
   *
   * @return line and column number of first location.
   */
  Location getFirst() const;

  /**
   * @brief Return the last location to apply mutation.
   *
   * @return line and column number of last location.
   */
  Location getLast() const;

  /**
   * @brief Return the token
   *
   * @return the token
   */
  std::string getToken() const;

  /**
   * @brief Return information of Mutant
   *
   * @return information of Mutant
   */
  std::string str() const;

 private:
  std::string mOperator;
  std::experimental::filesystem::path mPath;
  std::string mClass;
  std::string mFunction;
  std::string mQualifiedFunction;
  Location mFirst;
  Location mLast;
  std::string mToken;
};

std::ostream& operator<<(std::ostream& out, const Mutant& m);
std::istream& operator>>(std::istream& in, Mutant &m);

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTANT_HPP_
