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

#ifndef INCLUDE_SENTINEL_MUTATIONRESULT_HPP_
#define INCLUDE_SENTINEL_MUTATIONRESULT_HPP_

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include "sentinel/Mutable.hpp"


namespace sentinel {

/**
 * @brief MutationResult class
 */
class MutationResult {
 public:
  /**
   * @brief Default constructor
   *
   * @param mut Mutable class' instance
   * @param killingTest that killed mutant
   * @param detected or not (True: detected, False: not)
   * @param indexOfMutableDB
   */
  explicit MutationResult(const sentinel::Mutable& mut, const std::string& killingTest,
                          bool detected, int indexOfMutableDB);

  /**
   * @brief Default constructor
   *
   * @param mutationResultFilePath
   * @throw InvalidArgumentExcpetion when mutationReulstFilePath doesn't have MutationResult
   */
  explicit MutationResult(const std::string& mutationResultFilePath);

  /**
   * @brief Return description of mutator.
   *
   * @return description of mutator
   */
  std::string getDescription() const;

  /**
   * @brief Return description of method which mutator is applied
   *
   * @return description of method which mutator is applied
   */
  std::string getMethodDescription() const;

  /**
   * @brief Return mutator name
   *
   * @return mutator name
   */
  std::string getMutator() const;

  /**
   * @brief Return name of class which mutator is applied
   *
   * @return name of class which mutator is applied
   */
  std::string getMutatedClass() const;

  /**
   * @brief Return name of method which mutator is applied
   *
   * @return name of method which mutator is applied
   */
  std::string getMutatedMethod() const;

  /**
   * @brief Return path of file which mutator is applied
   *
   * @return path of file which mutator is applied
   */
  std::string getPath() const;

  /**
   * @brief Return killingTest that killed mutant
   *
   * @return killingTest that killed mutant
   */
  std::string getKillingTest() const;

  /**
   * @brief Return bool value to check if mutant is dead
   *
   * @return bool value to check if mutant is dead
   */
  bool getDetected() const;

  /**
   * @brief Return number of line which mutator is applied
   *
   * @return number of line which mutator is applied
   */
  int getLineNum() const;

  /**
   * @brief Return index of mutable db
   *
   * @return index of mutable db
   */
  int getIndexOfMutableDB() const;

  /**
   * @brief get last modifed time
   *
   * @return last modified time if instance is loaded from file (if not -1)
   */
  std::time_t getLastModifiedTime() const;

  /**
   * @brief write instance's contents to file
   *
   * @param dirPath
   *
   * @throw InvalidArgumentException if dirPath is not directory
   */
  void saveToFile(const std::string& dirPath) const;

  /**
   * @brief compare this with other
   *
   * @param other
   *
   * @return bool value whether two MutationReults are same
   */
  bool compare(const MutationResult& other) const;

 private:
  /**
   * @brief read String from file
   *
   * @param inFile input file stream
   */
  std::string readStringFromFile(std::ifstream& inFile);

  /**
   * @brief wrinte String to file
   *
   * @param outFile output file stream
   * @param outString target string to be written
   */
  void writeStringToFile(std::ofstream& outFile,
                         const std::string& outString) const;

  std::string mMethodDescription;
  std::string mMutator;
  std::string mMutatedClass;
  std::string mMutatedMethod;
  std::string mPath;
  std::string mKillingTest;
  bool mDetected;
  int mLineNum;
  int mIndexOfMutableDB;
  std::time_t mLastModified = -1;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONRESULT_HPP_
