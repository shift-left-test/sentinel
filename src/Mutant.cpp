/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <yaml-cpp/yaml.h>
#include <filesystem>  // NOLINT
#include <iterator>
#include <string>
#include "sentinel/Mutant.hpp"

namespace sentinel {

namespace fs = std::filesystem;

Mutant::Mutant() : mFirst{0, 0}, mLast{0, 0} {
}

Mutant::Mutant(const std::string& mutationOperator, const std::filesystem::path& path,
               const std::string& qualifiedFuncName, std::size_t firstLine, std::size_t firstColumn,
               std::size_t lastLine, std::size_t lastColumn, const std::string& token) :
    mOperator(mutationOperator),
    mPath(path),
    mQualifiedFunction(qualifiedFuncName),
    mFirst{firstLine, firstColumn},
    mLast{lastLine, lastColumn},
    mToken(token) {
  std::size_t pos = qualifiedFuncName.find_last_of(':');
  if (pos == std::string::npos) {
    mClass = "";
    mFunction = qualifiedFuncName;
  } else {
    mClass = qualifiedFuncName.substr(0, pos);
    mFunction = qualifiedFuncName.substr(pos + 1);
  }
}

bool Mutant::operator==(const Mutant& other) const {
  return mOperator == other.getOperator() && mPath == other.getPath() &&
         mQualifiedFunction == other.getQualifiedFunction() &&
         mFirst.line == other.getFirst().line && mFirst.column == other.getFirst().column &&
         mLast.line == other.getLast().line && mLast.column == other.getLast().column && mToken == other.getToken();
}

bool Mutant::operator<(const Mutant& other) const {
  if (mPath != other.mPath) return mPath < other.mPath;
  if (mFirst.line != other.mFirst.line) return mFirst.line < other.mFirst.line;
  if (mFirst.column != other.mFirst.column) return mFirst.column < other.mFirst.column;
  if (mLast.line != other.mLast.line) return mLast.line < other.mLast.line;
  if (mLast.column != other.mLast.column) return mLast.column < other.mLast.column;
  if (mOperator != other.mOperator) return mOperator < other.mOperator;
  if (mQualifiedFunction != other.mQualifiedFunction) return mQualifiedFunction < other.mQualifiedFunction;
  return mToken < other.mToken;
}

bool Mutant::operator!=(const Mutant& other) const {
  return !(*this == other);
}

const std::string& Mutant::getOperator() const {
  return mOperator;
}

const std::filesystem::path& Mutant::getPath() const {
  return mPath;
}

const std::string& Mutant::getClass() const {
  return mClass;
}

const std::string& Mutant::getFunction() const {
  return mFunction;
}

const std::string& Mutant::getQualifiedFunction() const {
  return mQualifiedFunction;
}

Location Mutant::getFirst() const {
  return mFirst;
}

Location Mutant::getLast() const {
  return mLast;
}

const std::string& Mutant::getToken() const {
  return mToken;
}

std::string Mutant::str() const {
  return fmt::format("{},{},{},{},{},{},{},{}", getOperator(), getPath().string(), getQualifiedFunction(),
                     getFirst().line, getFirst().column, getLast().line, getLast().column, getToken());
}

std::ostream& operator<<(std::ostream& out, const Mutant& m) {
  YAML::Emitter emitter;
  emitter << YAML::BeginMap;
  emitter << YAML::Key << "operator" << YAML::Value << m.getOperator();
  emitter << YAML::Key << "path" << YAML::Value << m.getPath().string();
  emitter << YAML::Key << "function" << YAML::Value << m.getQualifiedFunction();
  emitter << YAML::Key << "first" << YAML::Value << YAML::Flow
          << YAML::BeginMap
          << YAML::Key << "line" << YAML::Value << m.getFirst().line
          << YAML::Key << "column" << YAML::Value << m.getFirst().column
          << YAML::EndMap;
  emitter << YAML::Key << "last" << YAML::Value << YAML::Flow
          << YAML::BeginMap
          << YAML::Key << "line" << YAML::Value << m.getLast().line
          << YAML::Key << "column" << YAML::Value << m.getLast().column
          << YAML::EndMap;
  emitter << YAML::Key << "token" << YAML::Value << m.getToken();
  emitter << YAML::EndMap;
  out << emitter.c_str();
  return out;
}

std::istream& operator>>(std::istream& in, Mutant& m) {
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
    m = Mutant(node["operator"].as<std::string>(),
               fs::path(node["path"].as<std::string>()),
               node["function"].as<std::string>(),
               node["first"]["line"].as<std::size_t>(),
               node["first"]["column"].as<std::size_t>(),
               node["last"]["line"].as<std::size_t>(),
               node["last"]["column"].as<std::size_t>(),
               node["token"].as<std::string>());
  } catch (const YAML::Exception&) {
    in.setstate(std::ios::failbit);
  }
  return in;
}

}  // namespace sentinel
