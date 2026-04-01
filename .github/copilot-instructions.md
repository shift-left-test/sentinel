# Copilot Instructions

Sentinel is a C++17 mutation testing tool for C/C++ projects using LLVM/Clang for AST-based mutation.

## Build & Test Commands

```bash
# Standard release build
cmake . && make all -j

# Development build (enables tests, static analysis, coverage, docs)
cmake -DCMAKE_TESTING_ENABLED=ON . && make all -j

# Run all tests
./test/unittest

# Run a specific test suite
./test/unittest --gtest_filter=ConfigTest.*

# Static analysis
make cppcheck cpplint

# Coverage report
gcovr -s -r . --object-directory .

# Doxygen docs
make doc

# Full CI build (build + doc + test + lint + coverage + package)
./build.sh
```

## Architecture

### Configuration Pipeline (`main.cpp`)

1. `CliConfigParser` → parses CLI args into a `Config`
2. `YamlConfigParser` → loads `sentinel.yaml` into a second `Config`
3. `ConfigResolver::resolve()` → merges: **CLI > YAML > built-in defaults**
4. `MutationRunner` → receives final resolved `Config`

`Config` uses `std::optional` for fields that can come from CLI or YAML, so the resolver can distinguish "not set" vs "set to default". CLI-only flags (`init`, `dryRun`, `verbose`, etc.) are plain `bool`.

### Core Pipeline (Chain of Responsibility)

`MutationRunner::run()` chains `Stage` subclasses via `setNext()`. Each stage implements `shouldSkip()`, `getPhase()`, and `execute()`. Shared state is passed as `PipelineContext*` (holds `Config`, `StatusLine`, `Workspace`).

Stages in order: `OriginalBuildStage` → `OriginalTestStage` → `GenerationStage` → `EvaluationStage` → `ReportStage` (with optional `DryRunStage`).

### Mutation Operators (`src/operators/`, `include/sentinel/operators/`)

Each operator (AOR, BOR, LCR, ROR, SDL, SOR, UOI) extends `MutationOperator` and implements a Clang AST visitor to emit `Mutant` objects. To add a new operator: add header in `include/sentinel/operators/`, implementation in `src/operators/`, and register in `MutationFactory`.

### Key Classes

| Class | Role |
|-------|------|
| `MutationFactory` | Walks Clang AST, collects candidates using all operators |
| `MutantGenerator` (Uniform/Random/Weighted) | Selects mutants from candidates |
| `GitRepository` | Wraps libgit2 for `--scope=commit` line diffs |
| `GitSourceTree` | Applies/reverts source patches |
| `GoogleTestXmlParser`, `QTestXmlParser`, `CTestXmlParser` | Parse JUnit-style XML test reports |
| `Workspace` | Manages output directories and persisted state |

Signal handlers restore source backups on abnormal exit.

## Code Conventions

- **Copyright header**: `Copyright (c) 2026 LG Electronics Inc.` (use current year in new files)
- **Header guards**: `INCLUDE_SENTINEL_<PATH>_HPP_`
- **Namespace**: `sentinel`; use `namespace fs = std::filesystem;` alias in `.cpp` files only (not in headers, not in function parameter types)
- **Member variables**: prefixed with `m` (e.g., `mConfig`)
- **`#include <filesystem>`**: always add `// NOLINT` on that line
- **Output parameters**: use pointers (`Config* cfg`), not non-const references
- **`std::optional`**: only where `nullopt` carries distinct meaning (e.g., `timeout`, `seed`, `threshold`, `partition`)
- **`= default` special members**: for classes with virtual functions, define them out-of-line in `.cpp` — inline `= default` in headers breaks gcov coverage measurement
- **Error handling**: `throw std::runtime_error(fmt::format(...))` for unrecoverable errors
- **No magic numbers/strings**: use `static constexpr` named constants
- Style must pass cppcheck, cpplint, Doxygen, and Google C++ Style Guide; max line length 120

## Docs & Config Sync

When changing user-facing behavior, options, or commands, update **all** of:
- `README.md` — options table and full config template
- `resources/man/sentinel.1.in` — man page source (cmake generates `sentinel.1`)
- `src/MutationRunner.cpp` — `kYamlTemplate` (template written by `sentinel --init`)
- `sample/sentinel.yaml` and `sample/README.md` — if sample workflow is affected

## Git

- Do not create commits automatically; leave staging and committing to the user.
- Single-line commit messages only. No body or bullet points.
