/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#include <cstdlib>
#include <cstring>
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
#include "sentinel/stages/BaselineTestStage.hpp"
#include "sentinel/stages/EvaluationStage.hpp"
#include "sentinel/util/signal.hpp"

namespace sentinel {

namespace fs = std::filesystem;

// --- Signal handler globals ---
static fs::path gBackupDir;
static fs::path gSourceRoot;
static fs::path gWorkspaceDir;
static StatusLine* gStatusLine = nullptr;

static void signalHandler(int signum);

void installSignalHandlers(StatusLine* sl, const Workspace& workspace) {
  gStatusLine = sl;
  gWorkspaceDir = workspace.getRoot();
  signal::setMultipleSignalHandlers(
      {SIGABRT, SIGINT, SIGFPE, SIGILL, SIGSEGV, SIGTERM, SIGQUIT, SIGHUP, SIGUSR1},
      signalHandler);
}

void clearSignalStatusLine() { gStatusLine = nullptr; }

static void signalHandler(int signum) {
  if (!gBackupDir.empty() && fs::is_directory(gBackupDir)) {
    EvaluationStage::restoreBackup(gBackupDir, gSourceRoot);
  }
  if (gStatusLine != nullptr) {
    gStatusLine->disable();
    gStatusLine = nullptr;
  }
  Console::flush();
  if (signum != SIGUSR1) {
    Console::err("Received signal: {}.", strsignal(signum));
    if (!gWorkspaceDir.empty()) {
      Console::err("  hint: Check logs in {} for details.", gWorkspaceDir.string());
    }
    std::exit(EXIT_FAILURE);
  }
}

// --- EvaluationStage ---

EvaluationStage::EvaluationStage(const Config& cfg, std::shared_ptr<StatusLine> sl,
                                 std::shared_ptr<Logger> log,
                                 std::shared_ptr<Workspace> workspace)
    : Stage(cfg, std::move(sl), std::move(log)), mWorkspace(std::move(workspace)) {}

bool EvaluationStage::execute() {
  if (mWorkspace->isComplete()) return true;  // already-complete: go to report

  auto indexedMutants = mWorkspace->loadMutants();
  mStatusLine->setTotalMutants(indexedMutants.size());
  mStatusLine->setPhase(StatusLine::Phase::MUTANT);

  // Determine timeout
  std::size_t computedTimeLimit = 0;
  if (*mConfig.timeout == "auto") {
    auto status = mWorkspace->loadStatus();
    computedTimeLimit = status.baselineTime.value_or(0);
  } else {
    computedTimeLimit = std::stoul(*mConfig.timeout);
  }
  const std::size_t killAfterSecs = std::stoul(*mConfig.killAfter);

  // Set backup globals now that workspace is known
  gBackupDir = mWorkspace->getBackupDir();
  gSourceRoot = *mConfig.sourceDir;

  const fs::path backupDir = mWorkspace->getBackupDir();
  const fs::path actualDir = mWorkspace->getActualDir();
  Evaluator evaluator(mWorkspace->getOriginalResultsDir(), *mConfig.sourceDir);

  for (const auto& [id, m] : indexedMutants) {
    if (mWorkspace->isDone(id)) {
      auto doneResult = mWorkspace->getDoneResult(id);
      evaluator.injectResult(doneResult);
      mStatusLine->recordResult(static_cast<int>(doneResult.getMutationState()));
      continue;
    }
    // isLocked: treat as incomplete — fall through to re-evaluate
    mWorkspace->setLock(id);
    mStatusLine->setMutantInfo(id, m.getOperator(),
                               m.getPath().filename().string(), m.getFirst().line);

    auto repo = std::make_unique<GitRepository>(*mConfig.sourceDir, *mConfig.extensions);
    repo->getSourceTree()->modify(m, backupDir.string());

    Subprocess mBuild(*mConfig.buildCmd, 0, 0,
                      mWorkspace->getMutantBuildLog(id).string(), *mConfig.silent);
    mBuild.execute();

    TestExecutionState testState = TestExecutionState::SUCCESS;
    if (mBuild.isSuccessfulExit()) {
      fs::remove_all(*mConfig.testResultDir);
      Subprocess mTest(*mConfig.testCmd, computedTimeLimit, killAfterSecs,
                       mWorkspace->getMutantTestLog(id).string(), *mConfig.silent);
      mTest.execute();
      if (mTest.isTimedOut()) {
        testState = TestExecutionState::TIMEOUT;
      } else {
        BaselineTestStage::copyTestReportTo(*mConfig.testResultDir, actualDir,
                                            *mConfig.testResultExts);
      }
    } else {
      testState = TestExecutionState::BUILD_FAILURE;
    }

    MutationResult result = evaluator.compare(m, actualDir, testState);
    restoreBackup(backupDir, *mConfig.sourceDir);
    mWorkspace->clearLock(id);
    mWorkspace->setDone(id, result);
    mStatusLine->recordResult(static_cast<int>(result.getMutationState()));
    fs::remove_all(actualDir);
  }

  mWorkspace->setComplete();
  return true;
}

void EvaluationStage::restoreBackup(const fs::path& backup, const fs::path& srcRoot) {
  for (const auto& dirent : fs::directory_iterator(backup)) {
    fs::copy(dirent.path(), srcRoot / dirent.path().filename(),
             fs::copy_options::overwrite_existing | fs::copy_options::recursive);
    fs::remove_all(dirent.path());
  }
}

}  // namespace sentinel
