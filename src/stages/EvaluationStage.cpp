/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <sys/wait.h>
#include <unistd.h>
#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/Console.hpp"
#include "sentinel/Evaluator.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Subprocess.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/stages/EvaluationStage.hpp"
#include "sentinel/util/io.hpp"

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
  mStatusLine->setTotalMutants(totalMutants);

  // Determine timeout
  std::size_t computedTimeLimit = 0;
  if (*mConfig.timeout == "auto") {
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

    MutationResult result = evaluateMutant(m, id, computedTimeLimit, killAfterSecs, evaluator);

    auto state = result.getMutationState();
    auto relPath = std::filesystem::canonical(m.getPath()).lexically_relative(sourceRoot);
    std::string token = m.getToken().empty() ? "DELETE" : fmt::format("\xe2\x86\x92 {}", m.getToken());
    Console::out("  [{:>{}}/{}] {} {:<13} {}  {}:{} ({})", current, fmt::formatted_size("{}", totalMutants),
                 totalMutants, stateIcon(state), MutationStateToStr(state), m.getOperator(), relPath,
                 m.getFirst().line, token);
    if (!result.getKillingTest().empty()) {
      Console::out("        killed by: {}", result.getKillingTest());
    }

    mWorkspace->clearLock(id);
    mWorkspace->setDone(id, result);
    mStatusLine->recordResult(static_cast<int>(result.getMutationState()));
  }

  mWorkspace->setComplete();
  return true;
}

MutationResult EvaluationStage::evaluateMutant(const Mutant& m, int id, std::size_t timeLimit,
                                               std::size_t killAfterSecs, Evaluator& evaluator) {
  const fs::path backupDir = mWorkspace->getBackupDir();
  const fs::path actualDir = mWorkspace->getActualDir();

  auto repo = std::make_unique<GitRepository>(*mConfig.sourceDir, *mConfig.extensions);
  repo->getSourceTree()->modify(m, backupDir.string());

  Subprocess mBuild(*mConfig.buildCmd, 0, 0, mWorkspace->getMutantBuildLog(id).string(), *mConfig.silent);
  mBuild.execute();

  TestExecutionState testState = TestExecutionState::SUCCESS;
  if (mBuild.isSuccessfulExit()) {
    fs::remove_all(*mConfig.testResultDir);
    Subprocess mTest(*mConfig.testCmd, timeLimit, killAfterSecs, mWorkspace->getMutantTestLog(id).string(),
                     *mConfig.silent);
    mTest.execute();
    if (mTest.isTimedOut()) {
      testState = TestExecutionState::TIMEOUT;
    } else {
      io::syncFiles(*mConfig.testResultDir, actualDir, *mConfig.testResultExts);
    }
  } else {
    testState = TestExecutionState::BUILD_FAILURE;
  }

  MutationResult result = evaluator.compare(m, actualDir, testState);
  mWorkspace->restoreBackup(*mConfig.sourceDir);
  fs::remove_all(actualDir);
  return result;
}

}  // namespace sentinel
