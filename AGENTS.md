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

# Run tests
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
| `GoogleTestXmlParser`, `QTestXmlParser`, `CTestXmlParser` | Parse JUnit-style XML test reports |

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
