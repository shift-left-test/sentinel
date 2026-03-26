/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/Console.hpp"
#include "sentinel/Evaluator.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Subprocess.hpp"
#include "sentinel/Timestamper.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/stages/EvaluationStage.hpp"
#include "sentinel/util/io.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

namespace fs = std::filesystem;

EvaluationStage::EvaluationStage(const Config& cfg, std::shared_ptr<StatusLine> sl,
                                 std::shared_ptr<Workspace> workspace) :
    Stage(cfg, std::move(sl)), mWorkspace(std::move(workspace)) {
}

bool EvaluationStage::shouldSkip() const {
  return mWorkspace->isComplete();
}

StatusLine::Phase EvaluationStage::getPhase() const {
  return StatusLine::Phase::EVALUATION;
}

static const char* stateIcon(MutationState state) {
  switch (state) {
    case MutationState::KILLED:
      return "\xe2\x9c\x97";       // ✗
    case MutationState::SURVIVED:
      return "\xe2\x9c\x93";       // ✓
    default:
      return "\xe2\x9a\xa0";       // ⚠
  }
}

bool EvaluationStage::execute() {
  auto indexedMutants = mWorkspace->loadMutants();
  std::size_t totalMutants = indexedMutants.size();
  Logger::info("Evaluating {} mutant{}...", totalMutants, totalMutants == 1 ? "" : "s");
  mStatusLine->setTotalMutants(totalMutants);

  // Determine timeout
  const bool isAutoTimeout = (*mConfig.timeout == "auto");
  std::size_t computedTimeLimit = 0;
  if (isAutoTimeout) {
    auto status = mWorkspace->loadStatus();
    computedTimeLimit = status.originalTime.value_or(0);
  } else {
    computedTimeLimit = std::stoul(*mConfig.timeout);
  }
  const std::size_t killAfterSecs = std::stoul(*mConfig.killAfter);

  Evaluator evaluator(mWorkspace->getOriginalResultsDir(), *mConfig.sourceDir);
  auto sourceRoot = std::filesystem::canonical(*mConfig.sourceDir);
  std::size_t current = 0;

  for (const auto& [id, m] : indexedMutants) {
    ++current;
    if (mWorkspace->isDone(id)) {
      auto doneResult = mWorkspace->getDoneResult(id);
      mStatusLine->recordResult(static_cast<int>(doneResult.getMutationState()));
      continue;
    }
    // isLocked: treat as incomplete — fall through to re-evaluate
    mWorkspace->setLock(id);
    mStatusLine->setMutantInfo(id, m.getOperator(), m.getPath().filename().string(), m.getFirst().line);

    auto detail = evaluateMutant(m, id, computedTimeLimit, killAfterSecs, &evaluator);
    const auto& result = detail.result;

    auto state = result.getMutationState();
    auto relPath = std::filesystem::canonical(m.getPath()).lexically_relative(sourceRoot);
    std::string token = m.getToken().empty() ? "DELETE" : fmt::format("\xe2\x86\x92 {}", m.getToken());
    std::string timing = fmt::format("  [build {}, test {}]",
                                      Timestamper::format(detail.buildSecs),
                                      Timestamper::format(detail.testSecs));
    Console::out("  [{:>{}}/{}] {} {:<13} {}  {}:{} ({}){}", current, fmt::formatted_size("{}", totalMutants),
                 totalMutants, stateIcon(state), MutationStateToStr(state), m.getOperator(), relPath,
                 m.getFirst().line, token, timing);
    if (!result.getKillingTest().empty()) {
      static constexpr std::size_t kMaxDisplayedTests = 2;
      auto tests = string::split(result.getKillingTest(), ", ");
      std::string summary = tests[0];
      for (std::size_t i = 1; i < std::min(tests.size(), kMaxDisplayedTests); ++i) {
        summary += ", " + tests[i];
      }
      if (tests.size() > kMaxDisplayedTests) {
        summary += fmt::format(" (+{} more)", tests.size() - kMaxDisplayedTests);
      }
      Console::out("          \xe2\x86\x90 {}", summary);
    }
    if (state == MutationState::BUILD_FAILURE) {
      Console::out("          \xe2\x86\xaa {}", mWorkspace->getMutantBuildLog(id).string());
    } else if (state == MutationState::RUNTIME_ERROR || state == MutationState::TIMEOUT) {
      Console::out("          \xe2\x86\xaa {}", mWorkspace->getMutantTestLog(id).string());
    }

    mWorkspace->clearLock(id);
    mWorkspace->setDone(id, result);
    mStatusLine->recordResult(static_cast<int>(result.getMutationState()));
  }

  mWorkspace->setComplete();
  return true;
}

EvaluationDetail EvaluationStage::evaluateMutant(const Mutant& m, int id, std::size_t timeLimit,
                                                 std::size_t killAfterSecs, Evaluator* evaluator) {
  const fs::path backupDir = mWorkspace->getBackupDir();
  const fs::path actualDir = mWorkspace->getActualDir();

  auto repo = std::make_unique<GitRepository>(*mConfig.sourceDir, *mConfig.extensions);
  repo->getSourceTree()->modify(m, backupDir.string());

  Timestamper buildTimer;
  Subprocess mBuild(*mConfig.buildCmd, 0, 0, mWorkspace->getMutantBuildLog(id).string(),
                    !isVerbose());
  mBuild.execute();
  const double buildSecs = buildTimer.toDouble();

  double testSecs = 0.0;
  TestExecutionState testState = TestExecutionState::SUCCESS;
  if (mBuild.isSuccessfulExit()) {
    fs::remove_all(*mConfig.testResultDir);
    Subprocess mTest(*mConfig.testCmd, timeLimit, killAfterSecs, mWorkspace->getMutantTestLog(id).string(),
                     !isVerbose());
    Timestamper testTimer;
    mTest.execute();
    testSecs = testTimer.toDouble();
    if (mTest.isTimedOut()) {
      testState = TestExecutionState::TIMEOUT;
    } else {
      io::syncFiles(*mConfig.testResultDir, actualDir, *mConfig.testResultExts);
    }
  } else {
    testState = TestExecutionState::BUILD_FAILURE;
  }

  MutationResult result = evaluator->compare(m, actualDir, testState);
  mWorkspace->restoreBackup(*mConfig.sourceDir);
  fs::remove_all(actualDir);
  return {result, buildSecs, testSecs};
}

}  // namespace sentinel
