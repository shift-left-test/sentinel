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

void setSignalStatusLine(StatusLine* sl) { gStatusLine = sl; }
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

EvaluationStage::EvaluationStage(const Config& cfg, StatusLine& sl,
                                 std::shared_ptr<Logger> log, fs::path workDir)
    : Stage(cfg, sl, std::move(log)), mWorkDir(std::move(workDir)) {}

bool EvaluationStage::execute() {
  Workspace ws(mWorkDir);
  if (ws.isComplete()) return true;  // already-complete: go to report

  auto indexedMutants = ws.loadMutants();
  mStatusLine.setTotalMutants(indexedMutants.size());
  mStatusLine.setPhase(StatusLine::Phase::MUTANT);

  // Determine timeout
  std::size_t computedTimeLimit = 0;
  if (*mConfig.timeout == "auto") {
    auto status = ws.loadStatus();
    computedTimeLimit = status.baselineTime.value_or(0);
  } else {
    computedTimeLimit = std::stoul(*mConfig.timeout);
  }
  const std::size_t killAfterSecs = std::stoul(*mConfig.killAfter);

  // Install signal handlers
  gBackupDir = ws.getBackupDir();
  gSourceRoot = *mConfig.sourceDir;
  gWorkspaceDir = mWorkDir;
  setSignalStatusLine(&mStatusLine);
  signal::setMultipleSignalHandlers(
      {SIGABRT, SIGINT, SIGFPE, SIGILL, SIGSEGV, SIGTERM, SIGQUIT, SIGHUP, SIGUSR1},
      signalHandler);

  const fs::path backupDir = ws.getBackupDir();
  const fs::path actualDir = mWorkDir / "actual";
  Evaluator evaluator(ws.getOriginalResultsDir(), *mConfig.sourceDir);

  for (const auto& [id, m] : indexedMutants) {
    if (ws.isDone(id)) {
      auto doneResult = ws.getDoneResult(id);
      evaluator.injectResult(doneResult);
      mStatusLine.recordResult(static_cast<int>(doneResult.getMutationState()));
      continue;
    }
    // isLocked: treat as incomplete — fall through to re-evaluate
    ws.setLock(id);
    mStatusLine.setMutantInfo(id, m.getOperator(),
                              m.getPath().filename().string(), m.getFirst().line);

    auto repo = std::make_unique<GitRepository>(*mConfig.sourceDir, *mConfig.extensions);
    repo->getSourceTree()->modify(m, backupDir.string());

    const fs::path mutantDir = ws.getMutantDir(id);
    Subprocess mBuild(*mConfig.buildCmd, 0, 0,
                      (mutantDir / "build.log").string(), *mConfig.silent);
    mBuild.execute();

    std::string testState = "success";
    if (mBuild.isSuccessfulExit()) {
      fs::remove_all(*mConfig.testResultDir);
      Subprocess mTest(*mConfig.testCmd, computedTimeLimit, killAfterSecs,
                       (mutantDir / "test.log").string(), *mConfig.silent);
      mTest.execute();
      if (mTest.isTimedOut()) {
        testState = "timeout";
      } else {
        BaselineTestStage::copyTestReportTo(*mConfig.testResultDir, actualDir,
                                           *mConfig.testResultExts);
      }
    } else {
      testState = "build_failure";
    }

    MutationResult result = evaluator.compare(m, actualDir, testState);
    restoreBackup(backupDir, *mConfig.sourceDir);
    ws.clearLock(id);
    ws.setDone(id, result);
    mStatusLine.recordResult(static_cast<int>(result.getMutationState()));
    fs::remove_all(actualDir);
  }

  ws.setComplete();
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
