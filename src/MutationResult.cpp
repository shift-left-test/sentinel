/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <yaml-cpp/yaml.h>
#include <iterator>
#include <sstream>
#include <string>
#include "sentinel/Mutant.hpp"
#include "sentinel/MutationResult.hpp"

namespace sentinel {

MutationResult::MutationResult(const Mutant& m, const std::string& killingTest, const std::string& errorTest,
                               MutationState state) :
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
  return mMutant == other.mMutant && mKillingTest == other.mKillingTest && mErrorTest == other.mErrorTest &&
         mState == other.mState;
}

std::ostream& operator<<(std::ostream& out, const MutationResult& mr) {
  std::ostringstream mutantYaml;
  mutantYaml << mr.getMutant();

  YAML::Emitter emitter;
  emitter << YAML::BeginMap;
  emitter << YAML::Key << "state" << YAML::Value << MutationStateToStr(mr.getMutationState());
  emitter << YAML::Key << "killing-test" << YAML::Value << mr.getKillingTest();
  emitter << YAML::Key << "error-test" << YAML::Value << mr.getErrorTest();
  emitter << YAML::Key << "mutant" << YAML::Value << YAML::Load(mutantYaml.str());
  emitter << YAML::EndMap;
  out << emitter.c_str();
  return out;
}

std::istream& operator>>(std::istream& in, MutationResult& mr) {
  std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (content.empty()) {
    in.setstate(std::ios::failbit);
    return in;
  }
  try {
    YAML::Node node = YAML::Load(content);
    if (!node.IsMap()) {
      in.setstate(std::ios::failbit);
      return in;
    }

    std::ostringstream mutantYaml;
    mutantYaml << node["mutant"];
    std::istringstream mutantStream(mutantYaml.str());
    Mutant m;
    if (!(mutantStream >> m)) {
      in.setstate(std::ios::failbit);
      return in;
    }

    mr = MutationResult(m,
                        node["killing-test"].as<std::string>(),
                        node["error-test"].as<std::string>(),
                        StrToMutationState(node["state"].as<std::string>()));
  } catch (const YAML::Exception&) {
    in.setstate(std::ios::failbit);
  } catch (const std::invalid_argument&) {
    in.setstate(std::ios::failbit);
  }
  return in;
}

}  // namespace sentinel
