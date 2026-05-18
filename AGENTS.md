# AGENTS.md

This file provides shared guidance for AI coding assistants working with this repository.

## Build Commands

```bash
# Standard release build
cmake .
make all -j

# Development build (tests, static analysis, coverage, docs)
cmake -DCMAKE_TESTING_ENABLED=ON .
find . -name "*.gcda" -delete  # prevent libgcov merge warnings
make all -j

# Run tests (parallel, recommended)
ctest --output-on-failure -j$(nproc)

# Or run the test binary directly (single-threaded)
./test/unittest

# Run a specific test (GoogleTest filter)
./test/unittest --gtest_filter=ConfigTest.*

# Run static analysis
make cppcheck cpplint

# Generate coverage report
gcovr -s -r . -e test -e external

# Measure branch coverage (excluding test directory)
gcovr -s -r . --txt-metric branch -e test -e external

# Generate Doxygen docs
make doc

# Generate Debian package
make package

# Full CI build (build + doc + test + static analysis + coverage + package)
./build.sh
```

## Architecture Overview

Sentinel is a C++17 mutation testing tool for C/C++ projects, using LLVM/Clang for AST-based mutation.

### Configuration Pipeline (`main.cpp`)

1. `Config::withDefaults()` provides built-in defaults
2. `YamlConfigParser::applyTo()` loads `sentinel.yaml` (or workspace `config.yaml` on resume)
3. `CliConfigParser::applyTo()` overlays CLI args
4. `ConfigValidator::validate()` checks the resolved config

Precedence: **CLI > YAML > built-in defaults**.

`Config` (`include/sentinel/Config.hpp`) uses `std::optional` for fields settable via CLI or YAML
so the resolver can distinguish "not set" from "set to default".
CLI-only control flags (`init`, `dryRun`, `verbose`, `force`, `clean`)
are plain `bool` and copied directly from CLI config.

### Core Pipeline — Chain of Responsibility (`main.cpp`)

The pipeline is assembled as a chain of `Stage` subclasses linked via `setNext()`.
Each stage implements `shouldSkip()`, `getPhase()`, and `execute()`.
Shared state flows through a `PipelineContext*` that holds `Config`, `StatusLine`, and `Workspace`.

Stage execution order:

```
OriginalBuildStage → OriginalTestStage → GenerationStage → DryRunStage → EvaluationStage → ReportStage
```

1. **OriginalBuildStage** — builds the project, verifies `compile_commands.json` exists
2. **OriginalTestStage** — runs tests to measure baseline time and validate test results
3. **GenerationStage** — walks the AST via `MutationFactory` and selects mutants using a `MutantGenerator`
4. **DryRunStage** — prints dry-run summary and stops the chain if `--dry-run` is set; otherwise passes through
5. **EvaluationStage** — for each mutant, applies the source patch, rebuilds, runs tests, and records Kill/Survive/BuildError/Timeout/Runtime
6. **ReportStage** — writes HTML and XML reports; exits with code 3 if mutation score is below threshold

Signal handlers restore source backups on abnormal exit.

### Partition Merge Mode (`--merge-partition`)

When invoked with `--merge-partition`, `main.cpp` skips the stage chain entirely
and runs `PartitionedWorkspaceMerger` to combine per-partition workspaces from
separate sentinel processes, then auto-generates the report. This is used to
parallelize a single mutation run across multiple processes (one per partition),
since per-file Clang frontend invocations cannot safely run multi-threaded
within one process (see Parsing Model).

### Mutation Operators (`src/operators/`, `include/sentinel/operators/`)

Each operator (AOR, BOR, LCR, ROR, SDL, SOR, UOI) extends `MutationOperator` and implements a Clang AST visitor that identifies replaceable tokens or statements and emits `Mutant` objects.

To add a new operator: add header in `include/sentinel/operators/`, implementation in `src/operators/`, and register in `MutationFactory`.

### Key Classes

| Class | Role |
|-------|------|
| `Stage` | Abstract base for pipeline stages; chains via `setNext()` |
| `PipelineContext` | Holds `Config`, `StatusLine`, `Workspace` — passed to every stage |
| `MutationFactory` | Walks Clang AST, collects mutation candidates using all operators |
| `MutantGenerator` (Uniform/Random/Weighted) | Selects mutants from candidates |
| `GitRepository` | Wraps libgit2 for `--scope=commit` line diffs |
| `GitSourceTree` | Applies/reverts source patches to the working tree |
| `Workspace` | Manages output directories, backup, and persisted run state |
| `StatusLine` | Terminal status display with phase/progress tracking |
| `ConfigValidator` | Validates the resolved `Config` before pipeline execution |
| `Evaluator` | Compares actual vs expected test results to determine mutation outcome |
| `OomHandler` | Routes LLVM OOM/fatal errors to a deterministic exit path (see Parsing Model) |
| `PartitionedWorkspaceMerger` | Merges per-partition workspaces produced by separate sentinel processes |
| `GoogleTestXmlParser`, `QTestXmlParser`, `CTestXmlParser` | Parse JUnit-style XML test reports |

### Parsing Model

`MutantGenerator` and its subclasses parse one source file at a time in
the main thread via `clang::tooling::ClangTool::run()`. LLVM/Clang's
library-level thread safety is not strong enough to safely run multiple
Clang frontend invocations within a single process (e.g.
`clang::HeaderSearch::LookupFile` and other `cl::opt` / `ManagedStatic`-backed
paths share process-global state); instead of working around that, sentinel
keeps parsing single-threaded and lets users run multiple sentinel processes
(one per partition) for concurrency. **Do not** reintroduce `std::async` /
threading around the per-file Clang frontend invocation.

`OomHandler` (`installOomHandlers()` in `main.cpp`) routes LLVM-detected
OOM and other unrecoverable LLVM errors — including `ClangTool::run()`'s
`llvm::report_fatal_error("Cannot chdir into ...")` when a compile-command
directory is missing — through a deterministic exit path: it emits a fixed
`ERROR:` message via `write(2)`, raises `SIGUSR1` so `SignalHandler` runs
the backup-restore / status-line cleanup callbacks, and then calls
`_exit(137)`. **Do not** use `Logger`, `Console`, `fmt`, or any allocating
call inside the OOM handler entry points (`onLlvmBadAlloc`, `onLlvmFatal`,
or the `set_new_handler` lambda) — they would re-enter the allocator and
re-trigger the same failure.

The cleanup callbacks dispatched via `SIGUSR1`
(`ws->restoreBackup`, `statusLine->disable`) **do allocate** through
`std::filesystem` and `fmt`. They run on a best-effort basis: a recursive
OOM during cleanup is short-circuited by `OomHandler::sInHandler` to
`_exit(137)`, which can leave the source tree partially mutated. Recovery
is the user's responsibility (`git checkout` or re-running sentinel) —
a fully allocation-free cleanup would require bypassing `std::filesystem`
for raw POSIX syscalls and is an accepted trade-off against code clarity.

### Git Integration

- `GitRepository` (wraps libgit2) — used for `--scope=commit` to get the diff of changed lines
- `GitSourceTree` — applies/reverts source patches to the working tree

### Key External Dependencies

| Library | Location | Purpose |
|---------|----------|---------|
| libgit2 | `external/libgit2/` | git operations |
| fmt | `external/fmt/` | string formatting |
| yaml-cpp | `external/yaml-cpp/` | YAML config parsing |
| tinyxml2 | `external/tinyxml2/` | XML test report parsing |
| args | `external/args-6.2.3/` | CLI argument parsing |
| ftxui | `external/ftxui/` | terminal UI rendering for `StatusLine` |

### Directory Layout

```
include/sentinel/   — public headers (mirrors src/ structure)
src/                — implementation files
src/operators/      — mutation operator implementations
src/stages/         — pipeline stage implementations
test/               — GoogleTest unit tests
cmake/              — CMake modules (FindClang, etc.)
external/           — vendored third-party libraries
resources/          — man page template
```

## Working Process

Before starting any code work, always follow these steps:

1. **Read before modifying** — Always read the relevant files and understand the existing structure before making any changes.
2. **Create a detailed plan** — Before writing any code, always draft a detailed work plan first. Include which files will be changed, the approach, and the expected scope of impact.
3. **Ask when unclear** — If requirements are ambiguous or a decision is needed, always ask before proceeding.
4. **Present options** — When there are multiple ways to implement something, show examples and trade-offs for each approach so the user can choose.
5. **Get approval before starting** — After sharing the plan, wait for the user's explicit approval before beginning any actual code work.

## Git

- **Do not create commits** — Never create git commits automatically. Leave staging and committing to the user.
- **Commit message style** — When asked to commit, write a single-line message that briefly describes the change. No body, no bullet points.

## Packaging

- The Debian package filename and the deb's internal `Version:` field encode
  the build environment so multiple variants of the same upstream release can
  coexist without colliding:
  `sentinel_<version>-1~<distroID><distroVer>+llvm<llvmMajor>_<arch>.deb`
  (e.g. `sentinel_1.0.0-1~ubuntu20.04+llvm14_amd64.deb`).
- The distro tag is read from `/etc/os-release`; the LLVM major comes from
  `llvm-config --version`. The package declares `Depends: clang-<llvmMajor>`
  so the matching toolchain (and its builtin headers) is pulled in by `apt`.
- When the packaging convention changes, update both `CMakeLists.txt`
  (CPack block) and the `README.md` install snippets together.

## Verification

- After completing code changes, run `./build.sh` to verify the full build (build + doc + test + static analysis + coverage + package).
- Run `make doc` to verify Doxygen documentation generates without warnings.
- If code changes affect user-facing behavior, options, or commands, update the following where applicable:
  - `README.md` — options table, full config template
  - `resources/man/sentinel.1.in` — man page (cmake generates `sentinel.1` from this file)
  - `src/YamlConfigWriter.cpp` — `kYamlTemplate` (the template written by `sentinel --init`)
  - `sample/sentinel.yaml` and `sample/README.md` — only if the sample config or workflow is affected
- When adding or modifying functionality, always add or update test cases in `test/` to cover the new or changed behavior.
- Unit tests must be compatible with GoogleTest 1.10. Do not use features introduced in later versions (e.g., `EXPECT_THAT` with `testing::ThrowsMessage`, `testing::WhenDynamicCastTo`, `GTEST_FLAG_SET`, etc.).

## Code Quality

Design principles to follow:

- Follow well-known coding principles: DRY, YAGNI, SOLID, KISS, Fail Fast, Law of Demeter, Boy Scout Rule.
- Remove unnecessary code — dead code, unused variables, redundant logic.
- Use clear, descriptive names — variable, function, and class names should convey intent without needing comments.

## Code Conventions

C++ syntax and style rules:

- Copyright year in new files must use the current year (e.g., `Copyright (c) 2026 LG Electronics Inc.`)
- Coding rules must satisfy cppcheck, cpplint, Doxygen, and Google C++ Style Guide
- Line width must not exceed 120 characters
- Header guard style: `INCLUDE_SENTINEL_<PATH>_HPP_`
- Namespace: `sentinel` (occasionally `sentinel::fs = std::filesystem`)
- Member variables prefixed with `m` (e.g., `mConfig`)
- `#include <filesystem>` must always be accompanied by `// NOLINT` (lint suppression for include order)
- `std::filesystem`: use the full form (`std::filesystem`) in headers; use the short alias (e.g., `namespace fs = std::filesystem; fs::...`) in implementation files, except for function parameter types which always use the full form
- Output parameters must use pointers (`Config* cfg`), not non-const references (`Config& cfg`), per Google C++ Style Guide
- C++17 throughout; `std::optional` used only where nullopt carries distinct meaning (e.g., `timeout`, `seed`, `threshold`, `partition`)
- Use modern C++ idioms — prefer range-based `for`, structured bindings, `auto`, smart pointers (`std::unique_ptr`, `std::shared_ptr`), `std::variant`/`std::optional` over raw pointers or C-style patterns
- Const correctness — apply `const` to variables, parameters, and member functions that do not modify state
- No magic numbers or strings — use named constants (e.g., `static constexpr`)
- Error handling — use `std::runtime_error` with `fmt::format` for error messages; throw on unrecoverable errors, propagate exceptions up the call stack
- For classes with virtual functions, `= default` definitions of special member functions (destructor, copy, move) must be placed out-of-line in the `.cpp` file — inline `= default` in headers causes gcov instrumentation conflicts across translation units, breaking coverage measurement.
