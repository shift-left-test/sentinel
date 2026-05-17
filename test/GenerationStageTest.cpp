/*
 * Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>  // NOLINT
#include <memory>
#include <stdexcept>
#include <string>
#include "helper/FileTestHelper.hpp"
#include "helper/TestTempDir.hpp"
#include "sentinel/Config.hpp"
#include "sentinel/Mutant.hpp"
#include "sentinel/PipelineContext.hpp"
#include "sentinel/StatusLine.hpp"
#include "sentinel/Workspace.hpp"
#include "sentinel/stages/GenerationStage.hpp"

namespace fs = std::filesystem;

using ::testing::HasSubstr;

namespace sentinel {

// GenerationStage holds shared_ptrs to a GitRepository and a MutantGenerator
// but only dereferences them inside execute() AFTER the compile_commands.json
// existence check. None of the assertions in this fixture reach that point —
// shouldSkip short-circuits one test, and the other two are killed by the
// missing-compile-db check — so passing nullptr collaborators is sufficient
// and keeps the fixture free of heavy libgit2 / Clang initialization.
class GenerationStageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mBase = testTempDir("SENTINEL_GENSTAGE_TEST");
    fs::remove_all(mBase);
    fs::create_directories(mBase);

    mWorkDir = mBase / "workspace";
    mCompileDbDir = mBase / "build";
    fs::create_directories(mCompileDbDir);

    mWorkspace = std::make_shared<Workspace>(mWorkDir);
    mWorkspace->initialize();

    mConfig.sourceDir = mBase;
    mConfig.workDir = mWorkDir;
    mConfig.compileDbDir = mCompileDbDir;
    mConfig.verbose = false;
  }

  void TearDown() override {
    fs::remove_all(mBase);
  }

  PipelineContext makeCtx() {
    return {mConfig, mStatusLine, *mWorkspace};
  }

  fs::path mBase;
  fs::path mWorkDir;
  fs::path mCompileDbDir;
  std::shared_ptr<Workspace> mWorkspace;
  Config mConfig;
  StatusLine mStatusLine;
};

TEST_F(GenerationStageTest, testShouldSkipWhenWorkspaceAlreadyHasMutants) {
  // Pre-populate the workspace with one mutant so hasMutants() returns true.
  // The stage's run() must short-circuit BEFORE execute() inspects the (absent)
  // compile_commands.json — proving shouldSkip() controls the gate.
  fs::path src = mBase / "foo.cpp";
  testutil::writeFile(src, "int foo() { return 0; }\n");
  Mutant m("AOR", src, "foo", 1, 1, 1, 5, "+");
  mWorkspace->createMutant(1, m);

  ASSERT_TRUE(mWorkspace->hasMutants());
  ASSERT_FALSE(fs::exists(mCompileDbDir / "compile_commands.json"));

  GenerationStage stage(nullptr, nullptr);
  auto ctx = makeCtx();
  EXPECT_NO_THROW(stage.run(&ctx));
}

TEST_F(GenerationStageTest, testExecuteThrowsWhenCompileCommandsJsonMissing) {
  ASSERT_FALSE(mWorkspace->hasMutants());
  ASSERT_FALSE(fs::exists(mCompileDbDir / "compile_commands.json"));

  GenerationStage stage(nullptr, nullptr);
  auto ctx = makeCtx();
  EXPECT_THROW(stage.run(&ctx), std::runtime_error);
}

TEST_F(GenerationStageTest, testExecuteErrorMessageGuidesUserToCompileDb) {
  // The error must mention the offending option/file so users know how to
  // recover. This guards against regression of the help text in
  // GenerationStage::execute().
  ASSERT_FALSE(fs::exists(mCompileDbDir / "compile_commands.json"));

  GenerationStage stage(nullptr, nullptr);
  auto ctx = makeCtx();
  try {
    stage.run(&ctx);
    FAIL() << "expected runtime_error";
  } catch (const std::runtime_error& e) {
    const std::string msg = e.what();
    EXPECT_THAT(msg, HasSubstr("compile_commands.json"));
    EXPECT_THAT(msg, HasSubstr(mCompileDbDir.string()));
  }
}

}  // namespace sentinel
