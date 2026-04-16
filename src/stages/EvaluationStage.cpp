/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/Console.hpp"
#include "sentinel/CoverageInfo.hpp"
#include "sentinel/Evaluator.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Subprocess.hpp"
#include "sentinel/Timestamper.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/stages/EvaluationStage.hpp"
#include "sentinel/util/Utf8Char.hpp"
#include "sentinel/util/io.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {

namespace fs = std::filesystem;

EvaluationStage::EvaluationStage(std::shared_ptr<GitRepository> repo) :
    mRepo(std::move(repo)) {
}

bool EvaluationStage::shouldSkip(const PipelineContext& ctx) const {
  return ctx.workspace.isComplete();
}

StatusLine::Phase EvaluationStage::getPhase() const {
  return StatusLine::Phase::EVALUATION;
}

bool EvaluationStage::execute(PipelineContext* ctx) {
  auto indexedMutants = ctx->workspace.loadMutants();
  std::size_t totalMutants = indexedMutants.size();
  Logger::info("Evaluating {} mutant{}...", totalMutants, totalMutants == 1 ? "" : "s");
  ctx->statusLine.setTotalMutants(totalMutants);

  // Determine timeout
  const bool isAutoTimeout = !ctx->config.timeout.has_value();
  std::size_t computedTimeLimit = 0;
  if (isAutoTimeout) {
    auto status = ctx->workspace.loadStatus();
    computedTimeLimit = status.originalTime.value_or(0);
  } else {
    computedTimeLimit = *ctx->config.timeout;
  }
  Evaluator evaluator(ctx->workspace.getOriginalResultsDir());

  std::vector<std::string> covFiles;
  std::transform(ctx->config.lcovTracefiles.begin(), ctx->config.lcovTracefiles.end(),
                 std::back_inserter(covFiles),
                 [](const fs::path& p) { return p.string(); });
  CoverageInfo coverageInfo(covFiles);
  const bool hasCoverage = !ctx->config.lcovTracefiles.empty();

  std::size_t current = 0;

  for (const auto& [id, m] : indexedMutants) {
    ++current;
    if (ctx->workspace.isDone(id)) {
      auto doneResult = ctx->workspace.getDoneResult(id);
      ctx->statusLine.recordResult(static_cast<int>(doneResult.getMutationState()));
      continue;
    }
    // isLocked: treat as incomplete — fall through to re-evaluate
    ctx->workspace.setLock(id);

    bool uncovered = false;
    if (hasCoverage) {
      const auto absPath = fs::canonical(ctx->config.sourceDir / m.getPath());
      uncovered = !coverageInfo.cover(absPath.string(), m.getFirst().line);
    }

    ctx->statusLine.setMutantInfo(current);

    MutationResult result = uncovered
        ? evaluator.compare(m, ctx->workspace.getActualDir(), TestExecutionState::UNCOVERED)
        : evaluateMutant(m, id, computedTimeLimit, &evaluator, ctx);

    static constexpr const char* kUncoveredLabel = "SURVIVED*";
    static constexpr const char* kUncoveredTiming = "  [no coverage]";
    const auto state = result.getMutationState();
    const auto relPath = m.getPath();
    const std::string token = m.getToken().empty()
        ? "DELETE" : fmt::format("{} {}", Utf8Char::ArrowRight, m.getToken());
    const char* label = uncovered ? kUncoveredLabel : mutationStateToStr(state);
    const std::string timing = uncovered ? kUncoveredTiming : fmt::format("  [{}/{}]",
        Timestamper::format(result.getBuildSecs()), Timestamper::format(result.getTestSecs()));
    Console::out("  [{:>{}}/{}] {} {:<13} {}  {}:{}:{} ({}){}", current,
                 fmt::formatted_size("{}", totalMutants), totalMutants,
                 mutationStateIcon(state), label, m.getOperator(),
                 relPath, m.getFirst().line, m.getFirst().column, token, timing);
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
      Console::out("          {} {}", Utf8Char::ArrowLeft, summary);
    }
    if (state == MutationState::BUILD_FAILURE) {
      Console::out("          {} {}", Utf8Char::ArrowHook, ctx->workspace.getMutantBuildLog(id));
    } else if (state == MutationState::RUNTIME_ERROR || state == MutationState::TIMEOUT) {
      Console::out("          {} {}", Utf8Char::ArrowHook, ctx->workspace.getMutantTestLog(id));
    }

    ctx->workspace.clearLock(id);
    ctx->workspace.setDone(id, result);
    ctx->statusLine.recordResult(static_cast<int>(result.getMutationState()));
  }

  ctx->workspace.setComplete();
  return true;
}

MutationResult EvaluationStage::evaluateMutant(const Mutant& m, int id, std::size_t timeLimit,
                                               Evaluator* evaluator, PipelineContext* ctx) {
  const fs::path backupDir = ctx->workspace.getBackupDir();
  const fs::path actualDir = ctx->workspace.getActualDir();

  mRepo->getSourceTree()->modify(m, backupDir.string());

  Timestamper buildTimer;
  Subprocess buildProc(ctx->config.buildCmd, 0, ctx->workspace.getMutantBuildLog(id).string(),
                       !isVerbose(*ctx));
  buildProc.execute();
  const double buildSecs = buildTimer.toDouble();

  double testSecs = 0.0;
  TestExecutionState testState = TestExecutionState::SUCCESS;
  if (buildProc.isSuccessfulExit()) {
    fs::remove_all(ctx->config.testResultDir);
    Subprocess testProc(ctx->config.testCmd, timeLimit, ctx->workspace.getMutantTestLog(id).string(),
                        !isVerbose(*ctx));
    Timestamper testTimer;
    testProc.execute();
    testSecs = testTimer.toDouble();
    if (testProc.isTimedOut()) {
      testState = TestExecutionState::TIMEOUT;
    } else if (testProc.isSignaled() || testProc.isSignalExit()) {
      testState = TestExecutionState::RUNTIME_ERROR;
    } else {
      io::syncFiles(ctx->config.testResultDir, actualDir);
    }
  } else {
    testState = TestExecutionState::BUILD_FAILURE;
  }

  MutationResult result = evaluator->compare(m, actualDir, testState);
  result.setBuildSecs(buildSecs);
  result.setTestSecs(testSecs);
  ctx->workspace.restoreBackup(ctx->config.sourceDir);
  fs::remove_all(actualDir);
  return result;
}

}  // namespace sentinel
