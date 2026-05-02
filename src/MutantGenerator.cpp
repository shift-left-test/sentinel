/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <clang/Basic/FileManager.h>
#include <clang/Basic/FileSystemOptions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/PCHContainerOperations.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <fmt/core.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <map>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/MutantGenerator.hpp"
#include "sentinel/operators/MutationOperator.hpp"
#include "sentinel/RandomMutantGenerator.hpp"
#include "sentinel/UniformMutantGenerator.hpp"
#include "sentinel/WeightedMutantGenerator.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace sentinel {

namespace fs = std::filesystem;

MutantGenerator::~MutantGenerator() = default;

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------
std::shared_ptr<MutantGenerator> MutantGenerator::getInstance(Generator generator,
                                                              const std::filesystem::path& directory) {
  switch (generator) {
    case Generator::UNIFORM:
      return std::make_shared<UniformMutantGenerator>(directory);
    case Generator::RANDOM:
      return std::make_shared<RandomMutantGenerator>(directory);
    case Generator::WEIGHTED:
      return std::make_shared<WeightedMutantGenerator>(directory);
  }
  throw InvalidArgumentException(
      fmt::format("Invalid value for generator option: {}", generatorToString(generator)));
}

// ---------------------------------------------------------------------------
// Template Method: generate()
// ---------------------------------------------------------------------------
Mutants MutantGenerator::generate(const SourceLines& sourceLines, std::size_t maxMutants,
                                  unsigned int randomSeed, std::size_t mutantsPerLine) {
  mCandidateCount = 0;
  mLinesByPath.clear();

  Mutants allMutants = collectAllMutants(sourceLines);
  CandidateIndex index = buildCandidateIndex(std::move(allMutants));
  return selectMutants(sourceLines, maxMutants, randomSeed, index, mutantsPerLine);
}

// ---------------------------------------------------------------------------
// loadCompilationDatabase — single source of truth for DB loading
// ---------------------------------------------------------------------------
std::unique_ptr<clang::tooling::CompilationDatabase>
MutantGenerator::loadCompilationDatabase() const {
  std::string errorMsg;
  auto compileDb = clang::tooling::CompilationDatabase::loadFromDirectory(mDbPath.string(), errorMsg);
  if (compileDb == nullptr) {
    throw IOException(EINVAL, errorMsg);
  }
  return compileDb;
}

// ---------------------------------------------------------------------------
// collectAllMutants — default implementation using shared AST visitor
// ---------------------------------------------------------------------------
Mutants MutantGenerator::collectAllMutants(const SourceLines& sourceLines) {
  Mutants mutables;
  auto compileDb = loadCompilationDatabase();

  std::map<fs::path, std::vector<std::size_t>> targetLines;
  for (const auto& sourceLine : sourceLines) {
    fs::path filename = sourceLine.getPath();
    targetLines[filename].push_back(sourceLine.getLineNumber());
  }

  const std::size_t totalFiles = targetLines.size();
  std::size_t doneFiles = 0;
  notifyProgress(doneFiles, totalFiles);

  for (const auto& [filename, lines] : targetLines) {
    try {
      auto factory = createActionFactory(&mutables, lines, mSelectedOperators);
      runClangToolForFile(*compileDb, filename, factory.get());
    } catch (const std::bad_alloc&) {
      rethrowAsOomError(filename);
    }
    ++doneFiles;
    notifyProgress(doneFiles, totalFiles);
  }

  return mutables;
}

// ---------------------------------------------------------------------------
// rethrowAsOomError — single source of truth for the OOM error message
// ---------------------------------------------------------------------------
void MutantGenerator::rethrowAsOomError(const std::filesystem::path& filename) {
  throw std::runtime_error(
      fmt::format("out of memory while generating mutants for '{}'", filename.string()));
}

// ---------------------------------------------------------------------------
// runClangToolForFile — chdir-free per-file frontend invocation
// ---------------------------------------------------------------------------
void MutantGenerator::runClangToolForFile(const clang::tooling::CompilationDatabase& db,
                                          const std::filesystem::path& filename,
                                          clang::tooling::FrontendActionFactory* actionFactory) {
  auto compileCmds = db.getCompileCommands(filename.string());
  if (compileCmds.empty()) {
    return;
  }

  auto pchOps = std::make_shared<clang::PCHContainerOperations>();
  for (auto& cmd : compileCmds) {
    std::vector<std::string> args = cmd.CommandLine;
    if (args.empty()) {
      continue;
    }
    args.insert(args.begin() + 1, "-working-directory=" + cmd.Directory);
    args.push_back("-ferror-limit=0");

    llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> vfs(llvm::vfs::createPhysicalFileSystem().release());
    llvm::IntrusiveRefCntPtr<clang::FileManager> files(
        new clang::FileManager(clang::FileSystemOptions{cmd.Directory}, vfs));

    clang::IgnoringDiagConsumer diagConsumer;
    clang::tooling::ToolInvocation invocation(args, actionFactory, files.get(), pchOps);
    invocation.setDiagnosticConsumer(&diagConsumer);
    invocation.run();
  }
}

// ---------------------------------------------------------------------------
// buildCandidateIndex
// ---------------------------------------------------------------------------
CandidateIndex MutantGenerator::buildCandidateIndex(Mutants mutants) {
  CandidateIndex index;
  std::sort(mutants.begin(), mutants.end());
  index.allMutants = std::move(mutants);

  for (const auto& m : index.allMutants) {
    index.mutantsByFile[m.getPath()].push_back(&m);
  }
  for (auto& entry : index.mutantsByFile) {
    std::sort(entry.second.begin(), entry.second.end(),
              [](const Mutant* lhs, const Mutant* rhs) { return *lhs < *rhs; });
  }

  return index;
}

// ---------------------------------------------------------------------------
// findCandidatesForLine — shared binary-search + filter helper
// ---------------------------------------------------------------------------
void MutantGenerator::findCandidatesForLine(const CandidateIndex& index, const std::filesystem::path& canonPath,
                                            std::size_t targetLine, std::vector<const Mutant*>* out) {
  out->clear();
  auto mutantsByFileIt = index.mutantsByFile.find(canonPath);
  if (mutantsByFileIt == index.mutantsByFile.end()) {
    return;
  }

  const auto& fileVec = mutantsByFileIt->second;
  auto endIt = std::upper_bound(fileVec.begin(), fileVec.end(), targetLine,
                                [](std::size_t t, const Mutant* m) { return t < m->getFirst().line; });

  std::copy_if(fileVec.begin(), endIt, std::back_inserter(*out),
               [targetLine](const Mutant* m) { return m->getLast().line >= targetLine; });
}

// ---------------------------------------------------------------------------
// SentinelASTVisitor
// ---------------------------------------------------------------------------
MutantGenerator::SentinelASTVisitor::SentinelASTVisitor(clang::ASTContext* context, Mutants* mutables,
                                                        const std::vector<std::size_t>& targetLines,
                                                        const std::vector<std::string>& selectedOps) :
    mContext(context), mSrcMgr(context->getSourceManager()), mMutants(mutables), mTargetLines(targetLines) {
  std::sort(mTargetLines.begin(), mTargetLines.end());
  initOperators(context, selectedOps);
}

MutantGenerator::SentinelASTVisitor::~SentinelASTVisitor() = default;

void MutantGenerator::SentinelASTVisitor::initOperators(clang::ASTContext* context,
                                                        const std::vector<std::string>& selectedOps) {
  mMutationOperators = createOperators(context, selectedOps);
}

bool MutantGenerator::SentinelASTVisitor::isOnTargetLine(std::size_t startLineNum, std::size_t endLineNum) const {
  auto lo = std::lower_bound(mTargetLines.begin(), mTargetLines.end(), startLineNum);
  return lo != mTargetLines.end() && *lo <= endLineNum;
}

void MutantGenerator::SentinelASTVisitor::populateMutants(clang::Stmt* s) {
  for (const auto& m : mMutationOperators) {
    if (m->canMutate(s)) {
      m->populate(s, mMutants);
    }
  }
}

bool MutantGenerator::SentinelASTVisitor::VisitStmt(clang::Stmt* s) {
  std::size_t startLineNum = 0;
  std::size_t endLineNum = 0;
  resolveExpansionLineRange(s, &mSrcMgr, mContext->getLangOpts(),
                            &startLineNum, &endLineNum);

  if (isOnTargetLine(startLineNum, endLineNum)) {
    populateMutants(s);
  }

  return true;
}

// ---------------------------------------------------------------------------
// SentinelASTConsumer
// ---------------------------------------------------------------------------
MutantGenerator::SentinelASTConsumer::SentinelASTConsumer(const clang::CompilerInstance& ci, Mutants* mutables,
                                                          const std::vector<std::size_t>& targetLines,
                                                          const std::vector<std::string>& selectedOps) :
    mMutants(mutables), mTargetLines(targetLines), mSelectedOps(selectedOps) {
}

void MutantGenerator::SentinelASTConsumer::HandleTranslationUnit(clang::ASTContext& context) {
  SentinelASTVisitor visitor(&context, mMutants, mTargetLines, mSelectedOps);
  visitor.TraverseDecl(context.getTranslationUnitDecl());
}

// ---------------------------------------------------------------------------
// GenerateMutantAction
// ---------------------------------------------------------------------------
MutantGenerator::GenerateMutantAction::GenerateMutantAction(Mutants* mutables,
                                                            const std::vector<std::size_t>& targetLines,
                                                            const std::vector<std::string>& selectedOps) :
    mMutants(mutables), mTargetLines(targetLines), mSelectedOps(selectedOps) {
}

std::unique_ptr<clang::ASTConsumer> MutantGenerator::GenerateMutantAction::CreateASTConsumer(
    clang::CompilerInstance& ci, llvm::StringRef inFile) {
  ci.getDiagnostics().setClient(new clang::IgnoringDiagConsumer());
  return std::make_unique<SentinelASTConsumer>(ci, mMutants, mTargetLines, mSelectedOps);
}

// ---------------------------------------------------------------------------
// createActionFactory
// ---------------------------------------------------------------------------
std::unique_ptr<clang::tooling::FrontendActionFactory> MutantGenerator::createActionFactory(
    Mutants* mutables, const std::vector<std::size_t>& targetLines,
    const std::vector<std::string>& selectedOps) {
  class SimpleFrontendActionFactory : public clang::tooling::FrontendActionFactory {
   public:
    SimpleFrontendActionFactory(Mutants* mutables, const std::vector<std::size_t>& targetLines,
                                const std::vector<std::string>& selectedOps) :
        mMutants(mutables), mTargetLines(targetLines), mSelectedOps(selectedOps) {
    }

#if LLVM_VERSION_MAJOR >= 10
    std::unique_ptr<clang::FrontendAction> create() override {
      return std::make_unique<GenerateMutantAction>(mMutants, mTargetLines, mSelectedOps);
    }
#else
    clang::FrontendAction* create() override {
      return new GenerateMutantAction(mMutants, mTargetLines, mSelectedOps);
    }
#endif

   private:
    Mutants* mMutants;
    const std::vector<std::size_t>& mTargetLines;
    const std::vector<std::string>& mSelectedOps;
  };

  return std::unique_ptr<clang::tooling::FrontendActionFactory>(
      new SimpleFrontendActionFactory(mutables, targetLines, selectedOps));
}

}  // namespace sentinel
