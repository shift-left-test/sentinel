# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Standard release build
cmake .
make all -j

# Development build (tests, static analysis, coverage, docs)
cmake -DENABLE_TEST=ON .
make all -j

# Run tests
./test/unittest

# Run a specific test (GoogleTest filter)
./test/unittest --gtest_filter=ConfigTest.*

# Full CI build (build + doc + test + coverage + package)
./build.sh

# Generate Debian package
make package

# Generate coverage report
make coverage

# Generate Doxygen docs
make doc
```

## Architecture Overview

Sentinel is a C++17 mutation testing tool for C/C++ projects, using LLVM/Clang for AST-based mutation.

### Configuration Pipeline (`main.cpp`)

1. `CliConfigParser` parses CLI args (via `args` library) into a `Config` struct
2. `YamlConfigParser` loads `sentinel.yaml` into a second `Config`
3. `ConfigResolver::resolve()` merges them: CLI > YAML > built-in defaults
4. `MutationRunner` receives the final resolved `Config`

`Config` (`include/sentinel/Config.hpp`) uses `std::optional` for fields settable via CLI or YAML
so the resolver can distinguish "not set" from "set to default".
CLI-only control flags (`init`, `dryRun`, `noStatusLine`, `verbose`, `force`, `clean`)
are plain `bool` and copied directly from CLI config.

### Core Pipeline (`MutationRunner`)

`MutationRunner::run()` orchestrates the full pipeline:
1. **Baseline** — builds and runs tests once to measure baseline time and validate test results
2. **Mutant generation** — `MutationFactory` uses Clang to walk the AST and collect mutation candidates; a `MutantGenerator` (Uniform/Random/Weighted) selects from candidates
3. **Evaluation** — for each mutant, applies the source patch, rebuilds, runs tests via `Subprocess`, parses XML test reports, and records Kill/Survive/BuildError/Timeout/Runtime
4. **Reporting** — `HTMLReport` and `XMLReport` write results; mutation score is printed to stderr

Signal handlers restore source backups on abnormal exit.

### Mutation Operators (`src/operators/`, `include/sentinel/operators/`)

Each operator (AOR, BOR, LCR, ROR, SDL, SOR, UOI) extends `MutationOperator` and implements a Clang AST visitor that identifies replaceable tokens or statements and emits `Mutant` objects.

### Git Integration

- `GitRepository` (wraps libgit2) — used for `--scope=commit` to get the diff of changed lines
- `GitSourceTree` — applies/reverts source patches to the working tree

### Test Result Parsers

`GoogleTestXmlParser`, `QTestXmlParser`, `CTestXmlParser` all extend `XmlParser` (tinyxml2) to parse JUnit-style XML output into `Result` objects.

### Key External Dependencies

| Library | Location | Purpose |
|---------|----------|---------|
| libgit2 | system | git operations |
| fmt | `external/fmt/` | string formatting |
| yaml-cpp | `external/yaml-cpp/` | YAML config parsing |
| tinyxml2 | `external/tinyxml2/` | XML test report parsing |
| args | `external/args-6.2.3/` | CLI argument parsing |

### Directory Layout

```
include/sentinel/   — public headers (mirrors src/ structure)
src/                — implementation files
src/operators/      — mutation operator implementations
test/               — GoogleTest unit tests
cmake/              — CMake modules (FindLibGit2, FindClang, etc.)
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

## Verification

- After completing code changes, run `./build.sh` to verify the full build (build + doc + test + coverage + package).
- If code changes affect user-facing behavior, options, or commands, update the following where applicable:
  - `README.md` — options table, full config template
  - `resources/man/sentinel.1.in` — man page (cmake generates `sentinel.1` from this file)
  - `src/MutationRunner.cpp` — `kYamlTemplate` (the template written by `--init`)
  - `sample/sentinel.yaml` and `sample/README.md` — only if the sample config or workflow is affected
- When adding or modifying functionality, always add or update test cases in `test/` to cover the new or changed behavior.

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
- C++17 throughout; `std::optional` used extensively in `Config`
- Use modern C++ idioms — prefer range-based `for`, structured bindings, `auto`, smart pointers (`std::unique_ptr`, `std::shared_ptr`), `std::variant`/`std::optional` over raw pointers or C-style patterns
- Const correctness — apply `const` to variables, parameters, and member functions that do not modify state
- No magic numbers or strings — use named constants (e.g., `static constexpr`)
- Error handling — use `std::runtime_error` with `fmt::format` for error messages; throw on unrecoverable errors, propagate exceptions up the call stack
