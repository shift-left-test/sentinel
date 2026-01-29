/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <sys/stat.h>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/MutationResult.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

MutationResult::MutationResult(const Mutant& m, const std::string& killingTest,
                               const std::string& errorTest, MutationState state) :
    mKillingTest(killingTest), mErrorTest(errorTest), mState(state), mMutant(m) {
}

std::string MutationResult::getKillingTest() const {
  return mKillingTest;
}

std::string MutationResult::getErrorTest() const {
  return mErrorTest;
}

MutationState MutationResult::getMutationState() const {
  return mState;
}

const Mutant& MutationResult::getMutant() const {
  return mMutant;
}

bool MutationResult::getDetected() const {
  return mState == MutationState::KILLED;
}

bool MutationResult::compare(const MutationResult& other) const {
  return mMutant == other.mMutant &&
      mKillingTest == other.mKillingTest &&
      mErrorTest == other.mErrorTest &&
      mState == other.mState;
}

std::ostream& operator<<(std::ostream& out, const MutationResult& mr) {
  out << fmt::format("{}\t{}\t{}\t\t\t", mr.getKillingTest(), mr.getErrorTest(),
                     static_cast<int>(mr.getMutationState()));
  out << mr.getMutant();
  return out;
}

std::istream& operator>>(std::istream& in, MutationResult &mr) {
  std::string line;
  if (getline(in, line)) {
    auto sep = string::split(line, "\t\t\t");
    auto str = string::split(sep[0], "\t");
    Mutant m;
    std::istringstream iss(sep[1]);
    iss >> m;
    mr = MutationResult(m, str[0], str[1], static_cast<MutationState>(string::stringToInt<int>(str[2])));
  }
  return in;
}

}  // namespace sentinel
