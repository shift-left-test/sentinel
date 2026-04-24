/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_MUTATIONRESULT_HPP_
#define INCLUDE_SENTINEL_MUTATIONRESULT_HPP_

#include <iosfwd>
#include <string>
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationState.hpp"

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
  MutationResult(const Mutant& m, const std::string& killingTest, const std::string& errorTest, MutationState state);

  /**
   * @brief Return killingTest that killed mutant
   *
   * @return killingTest that killed mutant
   */
  const std::string& getKillingTest() const;

  /**
   * @brief Return errorTest that occur runtime error when testing
   *
   * @return errorTest that killed mutant
   */
  const std::string& getErrorTest() const;

  /**
   * @brief Return MutationState
   *
   * @return MutationState
   */
  MutationState getMutationState() const;

  /**
   * @brief Return the associated Mutant object
   *
   * @return the associated Mutant object
   */
  const Mutant& getMutant() const;

  /**
   * @brief Return the build duration in seconds
   *
   * @return build duration in seconds
   */
  double getBuildSecs() const;

  /**
   * @brief Return the test duration in seconds
   *
   * @return test duration in seconds
   */
  double getTestSecs() const;

  /**
   * @brief Set the build duration in seconds
   *
   * @param secs build duration in seconds
   */
  void setBuildSecs(double secs);

  /**
   * @brief Set the test duration in seconds
   *
   * @param secs test duration in seconds
   */
  void setTestSecs(double secs);

  /**
   * @brief compare this with other
   *
   * @param other
   *
   * @return bool value whether two MutationResults are same
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
  MutationState mState = MutationState::SURVIVED;
  Mutant mMutant;
  double mBuildSecs = 0.0;
  double mTestSecs = 0.0;
};

std::ostream& operator<<(std::ostream& out, const MutationResult& mr);
std::istream& operator>>(std::istream& in, MutationResult& mr);

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONRESULT_HPP_
