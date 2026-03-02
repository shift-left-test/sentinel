# Sentinel

**Sentinel** is a mutation testing tool based on LLVM/Clang for C/C++ projects.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.4.8-blue.svg)](CMakeLists.txt)

---

## Table of Contents

- [About](#about)
- [Quick Start](#quick-start)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration File](#configuration-file)
- [Mutation Operators](#mutation-operators)
- [Supported Test Runners](#supported-test-runners)
- [Development](#development)
- [Licenses](#licenses)

---

## About

Mutation testing evaluates the quality of a test suite by injecting small, deliberate faults (*mutants*) into the source code and checking whether the tests detect them.

**Benefits:**
- Measure test effectiveness with a *mutation score* (killed / (killed + survived))
- Identify undertested code paths and guide test improvements

Sentinel makes mutation testing practical for C/C++ projects by:
- Integrating with git to limit mutations to changed lines (`--scope=commit`)
- Automatically generating HTML/XML reports
- Supporting resume after interruption

> For background, see [Mutation testing on Wikipedia](https://en.wikipedia.org/wiki/Mutation_testing).

---

## Quick Start

```bash
# 1. Start the Docker environment
git clone https://github.com/shift-left-test/dockerfiles.git
cd dockerfiles
docker build -f clang-dev/11/Dockerfile -t clang-dev-11 .
docker run --rm -it clang-dev-11

# 2. Build and install Sentinel
git clone https://github.com/shift-left-test/sentinel
cd sentinel && cmake . && make all -j && make package
sudo apt-get install ./sentinel-0.4.8-amd64.deb

# 3. Run mutation testing on your project
sentinel /path/to/your/project \
  --build-command="make" \
  --test-command="make test" \
  --test-result-dir=./test-results
```

---

## Prerequisites

| Requirement | Version  |
|-------------|----------|
| LLVM/Clang  | 11+      |
| CMake       | 3.8+     |
| libgit2     | any      |
| Docker      | optional (recommended for setup) |

---

## Installation

### Option 1: Docker (Recommended)

Use the preconfigured Docker image to avoid manual dependency setup.

```bash
git clone https://github.com/shift-left-test/dockerfiles.git
cd dockerfiles
docker build -f clang-dev/11/Dockerfile -t clang-dev-11 .
docker run --rm -it clang-dev-11
```

### Option 2: Build from Source

```bash
git clone https://github.com/shift-left-test/sentinel
cd sentinel
cmake .
make all -j
make package
sudo apt-get install ./sentinel-0.4.8-amd64.deb
```

---

## Usage

```
sentinel [SOURCE_ROOT_PATH] {OPTIONS}
```

Sentinel builds the project, runs the test suite once to establish a baseline, then applies each mutant in turn and checks whether the tests catch it.

### Example

```bash
sentinel ./my-project \
  --build-command="cmake --build build" \
  --test-command="ctest --test-dir build" \
  --test-result-dir=./build/test-results \
  --scope=commit \
  --limit=50 \
  --exclude="*/third_party/*" "*/test/*"
```

### Terminal Status Line

When stdout is a TTY, Sentinel displays a live status line at the bottom of the terminal:

```
 Phase:MUTANT     [42/150] | AOR  sample.cpp:10 | K:30 S:8 B:2 T:1 R:0 | Score:73.2% | Elapsed:00:42:17
```

It is suppressed automatically when output is piped or redirected. Use `--no-statusline` to disable it explicitly.

### Resume

If Sentinel is interrupted, rerun it with the same `--workspace` path. It will detect the previous run and prompt to resume.

---

### Option Reference

#### General

| Option | Description | Default |
|--------|-------------|---------|
| `SOURCE_ROOT_PATH` | Root of the source tree to test | `.` |
| `-v, --verbose` | Enable verbose output (INFO level and above) | |
| `--debug` | Enable debug output (all log levels) | |
| `-w, --workspace=PATH` | Directory for all run artifacts | `./sentinel_workspace` |
| `-o, --output-dir=PATH` | Directory to write HTML/XML reports | |
| `--cwd=PATH` | Change to this directory before running | |

#### Run options

| Option | Description | Default |
|--------|-------------|---------|
| `--config=PATH` | YAML config file path | `sentinel.yaml` (auto-detected) |
| `--init` | Write a `sentinel.yaml` config template and exit | |
| `--no-statusline` | Disable the live terminal status line | |

#### Build & test options

| Option | Description | Default |
|--------|-------------|---------|
| `-b, --build-dir=PATH` | Build output directory (also searched for `compile_commands.json`) | `.` |
| `--compiledb=PATH` | Directory containing `compile_commands.json` (overrides `--build-dir`) | |
| `--build-command=CMD` | Shell command to build the project | **required** |
| `--test-command=CMD` | Shell command to run tests | **required** |
| `--test-result-dir=PATH` | Directory where the test command writes result files | **required** |
| `--test-result-extension=EXT` | File extension of test result files (repeatable) | `xml` |
| `--timeout=SEC` | Test time limit; `0` = no limit, `auto` = 2× baseline run time | `auto` |
| `--kill-after=SEC` | Seconds after timeout before sending SIGKILL (0 = disabled) | `60` |

#### Mutation options

| Option | Description | Default |
|--------|-------------|---------|
| `-s, --scope=SCOPE` | `commit` (changed lines only) or `all` (entire codebase) | `all` |
| `-t, --extension=EXT` | Source file extensions to mutate (repeatable) | `cxx cpp cc c c++ cu` |
| `-p, --pattern=PATTERN` | Paths or glob patterns to constrain the mutation scope (repeatable) | |
| `-e, --exclude=PATTERN` | Exclude files/directories matching fnmatch-style patterns (repeatable) | |
| `-l, --limit=N` | Maximum number of mutants to generate | `10` |
| `--generator=TYPE` | Mutant selection strategy: `uniform`, `random`, or `weighted` | `uniform` |
| `--seed=N` | Random seed for mutant selection | random |
| `--operator=OP` | Mutation operators to apply (repeatable; defaults to all) | all |
| `--coverage=FILE` | lcov coverage info file; limits mutation to covered lines (repeatable) | |

---

## Configuration File

Sentinel supports a YAML configuration file (`sentinel.yaml`) as an alternative to passing all options on the command line.

When invoked, Sentinel looks for `sentinel.yaml` in the current working directory automatically. Use `--config` to specify a different file, or run `sentinel --init` to generate a template.

**Precedence:**
```
CLI argument  >  sentinel.yaml  >  built-in default
```

### Minimal Example

```yaml
# sentinel.yaml
source-root: ./src
build-dir: ./build
build-command: cmake --build build
test-command: ctest --test-dir build
test-result-dir: ./build/test-results
scope: commit
limit: 50
exclude:
  - "*/third_party/*"
  - "*/test/*"
```

### Full Template

Run `sentinel --init` to write a fully commented template to the current directory, or see the template below.

```yaml
# sentinel.yaml — full configuration template
# CLI arguments always take priority over values in this file.

# Root of the source tree to test
# source-root: ./

# Enable verbose or debug logging
# verbose: false
# debug: false

# Directory for all run artifacts
# workspace: ./sentinel_workspace

# Directory to write HTML/XML reports
# output-dir: ./sentinel_output

# Change to this directory before running
# cwd: .

# --- Build & test ---

# Build output directory (also searched for compile_commands.json by default)
# build-dir: .

# Directory containing compile_commands.json (overrides build-dir)
# compiledb: .

# Shell command to build the project
# build-command: make

# Shell command to run tests
# test-command: make test

# Directory where the test command writes result files
# test-result-dir: ./test-results

# File extensions of test result files
# test-result-extension:
#   - xml

# Test time limit in seconds (0 = no limit, auto = 2x baseline run time)
# timeout: auto

# Seconds after timeout before sending SIGKILL (0 = disabled)
# kill-after: 60

# --- Mutation ---

# Mutation scope: 'commit' (changed lines only) or 'all' (entire codebase)
# scope: all

# Source file extensions to mutate
# extension:
#   - cpp
#   - cxx
#   - cc
#   - c
#   - c++
#   - cu

# Paths or glob patterns to constrain the mutation scope
# pattern: []

# Exclude files/directories matching fnmatch-style patterns
# exclude: []

# Maximum number of mutants to generate
# limit: 10

# Mutant selection strategy: 'uniform', 'random', or 'weighted'
# generator: uniform

# Random seed for mutant selection
# seed: 42

# Mutation operators to apply (defaults to all)
# Valid values: AOR, BOR, LCR, ROR, SDL, SOR, UOI
# operator:
#   - AOR
#   - BOR
#   - LCR
#   - ROR
#   - SDL
#   - SOR
#   - UOI

# lcov coverage info files; limits mutation to covered lines
# coverage: []

# Disable the terminal status line even when stdout is a TTY
# no-statusline: false
```

---

## Mutation Operators

| Operator | Name | Description | Example |
|----------|------|-------------|---------|
| **AOR** | Arithmetic Operator Replacement | Replaces arithmetic operators | `a + b` → `a - b` |
| **BOR** | Bitwise Operator Replacement | Replaces bitwise operators | `a & b` → `a \| b` |
| **LCR** | Logical Connector Replacement | Replaces logical operators | `a && b` → `a \|\| b` |
| **ROR** | Relational Operator Replacement | Replaces relational operators | `a < b` → `a > b` |
| **SDL** | Statement Deletion | Deletes a statement body | `return x;` → *(deleted)* |
| **SOR** | Shift Operator Replacement | Replaces shift operators | `a << b` → `a >> b` |
| **UOI** | Unary Operator Insertion | Inserts a unary operator | `x` → `-x` |

---

## Supported Test Runners

- [GoogleTest](https://github.com/google/googletest)
- [QTest](https://doc.qt.io/qt-6/qtest-overview.html)
- [CTest](https://cmake.org/cmake/help/latest/manual/ctest.1.html)

---

## Development

To build with tests and static analysis enabled:

```bash
cmake -DENABLE_TEST=ON .
make all -j
./test/unittest
```

This enables:
- Static analysis (`cppcheck`, `cpplint`, `lwyu`)
- Unit tests
- Code coverage
- Doxygen documentation

---

## Licenses

The project source code is available under the MIT license. See [LICENSE](LICENSE).

Third-party licenses are listed in the [LICENSES](LICENSES) directory.
