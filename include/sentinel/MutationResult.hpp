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

#include <fstream>
#include <iostream>
#include <string>
#include "sentinel/MutationState.hpp"
#include "sentinel/Mutant.hpp"

namespace sentinel {

/**
 * @brief MutationResult class
 */
class MutationResult {
 public:
  /**
   * @brief Default constructor
   */
  MutationResult() = default;

  /**
   * @brief Default constructor
   *
   * @param m Mutant class' instance
   * @param killingTest that killed mutant
   * @param errorTest that killed mutant
   * @param state MutationState
   */
  MutationResult(const Mutant& m, const std::string& killingTest,
                 const std::string& errorTest, MutationState state);

  /**
   * @brief Return killingTest that killed mutant
   *
   * @return killingTest that killed mutant
   */
  std::string getKillingTest() const;

  /**
   * @brief Return errorTest that occur runtime error when testing
   *
   * @return errorTest that killed mutant
   */
  std::string getErrorTest() const;

  /**
   * @brief Return MutationState
   *
   * @return MutationState
   */
  MutationState getMutationState() const;

  /**
   * @brief Return bool value to check if mutant is dead
   *
   * @return bool value to check if mutant is dead
   */
  const Mutant& getMutant() const;

  /**
   * @brief compare this with other
   *
   * @param other
   *
   * @return bool value whether two MutationReults are same
   */
  bool compare(const MutationResult& other) const;

  /**
   * @brief Return bool value to check if mutant is dead
   *
   * @return bool value to check if mutant is dead
   */
  bool getDetected() const;

 private:
  std::string mKillingTest;
  std::string mErrorTest;
  MutationState mState;
  Mutant mMutant;
};

std::ostream& operator<<(std::ostream& out, const MutationResult& mr);
std::istream& operator>>(std::istream& in, MutationResult &mr);

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONRESULT_HPP_
