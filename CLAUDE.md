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

`Config` (`include/sentinel/Config.hpp`) uses `std::optional` for every field so the resolver can distinguish "not set" from "set to default".

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
resources/          — man page template, YAML config template
```

## Working Process

Before starting any code work, always follow these steps:

1. **Create a detailed plan** — Before writing any code, always draft a detailed work plan first. Include which files will be changed, the approach, and the expected scope of impact.
2. **Ask when unclear** — If requirements are ambiguous or a decision is needed, always ask before proceeding.
3. **Present options** — When there are multiple ways to implement something, show examples and trade-offs for each approach so the user can choose.
4. **Get approval before starting** — After sharing the plan, wait for the user's explicit approval before beginning any actual code work.

## Verification

- After completing code changes, run `./build.sh` to verify the full build (build + doc + test + coverage + package).
- If code changes affect user-facing behavior, options, or commands, update `README.md` and the man page (`resources/`) accordingly.

## Code Conventions

- Coding rules must satisfy cppcheck, cpplint, Doxygen, and Google C++ Style Guide
- Line width must not exceed 120 characters
- Header guard style: `INCLUDE_SENTINEL_<PATH>_HPP_`
- Namespace: `sentinel` (occasionally `sentinel::fs = std::filesystem`)
- Member variables prefixed with `m` (e.g., `mConfig`)
- `#include <filesystem>` must always be accompanied by `// NOLINT` (lint suppression for include order)
- `std::filesystem`: use the full form (`std::filesystem`) in headers; use the short alias (e.g., `namespace fs = std::filesystem; fs::...`) in implementation files, except for function parameter types which always use the full form
- C++17 throughout; `std::optional` used extensively in `Config`
