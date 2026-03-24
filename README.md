# Sentinel

**Sentinel** is a mutation testing tool based on LLVM/Clang for C/C++ projects.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.4.8-blue.svg)](CMakeLists.txt)

---

## Table of Contents

- [About](#about)
- [Quick Start](#quick-start)
- [Sample Project](#sample-project)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration File](#configuration-file)
- [Coverage-Guided Mutation](#coverage-guided-mutation)
- [Mutation Operators](#mutation-operators)
- [Supported Test Result Formats](#supported-test-result-formats)
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

### 1. Start the Docker environment

```bash
git clone https://github.com/shift-left-test/sentinel
cd sentinel
docker build -t sentinel-dev .
docker run --rm -it -v $(pwd):/workspace sentinel-dev
```

### 2. Build and install Sentinel

Inside the Docker container:

```bash
cd /workspace && cmake . && make all -j && make package
sudo apt-get install ./sentinel-0.4.8-amd64.deb
```

### 3. Try it on the sample project

```bash
cd sentinel/sample
sentinel
```

Sentinel will build the project, run the tests, evaluate mutants, and write an HTML report to `./sentinel_output/`. See [Sample Project](#sample-project) for details.

---

## Sample Project

The `sample/` directory contains a ready-to-use C++ project pre-configured for mutation testing — the fastest way to see Sentinel in action without any setup on your own project.

```bash
cd sample
sentinel
```

Sentinel will build the project, run the test suite, evaluate mutants, and write an HTML report to `./sample/sentinel_output/`. See [`sample/README.md`](sample/README.md) for details.

---

## Prerequisites

| Requirement | Version  |
|-------------|----------|
| LLVM/Clang  | 11+      |
| CMake       | 3.13+    |
| libgit2     | any      |
| Docker      | optional (recommended for setup) |

---

## Installation

### Option 1: Docker (Recommended)

Use the preconfigured Docker image to avoid manual dependency setup.

```bash
git clone https://github.com/shift-left-test/sentinel
cd sentinel
docker build -t sentinel-dev .
docker run --rm -it -v $(pwd):/workspace sentinel-dev
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
sentinel {OPTIONS}
```

Sentinel builds the project, runs the test suite once to establish a baseline, then applies each mutant in turn and checks whether the tests catch it.

### With sentinel.yaml (Recommended)

Place a `sentinel.yaml` in your project directory and run `sentinel` from there:

```yaml
# sentinel.yaml
version: 1
source-dir: ./src
build-command: cmake -B build && cmake --build build
compiledb-dir: ./build
test-command: ctest --test-dir build
test-result-dir: ./build/test-results
scope: commit
limit: 50
exclude:
  - "*/third_party/*"
  - "*/test/*"
```

```bash
sentinel
```

Run `sentinel --init` to generate a fully commented template in the current directory.

### With CLI Options

All settings can be passed directly on the command line. CLI options take priority over `sentinel.yaml`.

```bash
sentinel \
  --source-dir=./my-project \
  --build-command="cmake -B build && cmake --build build" \
  --test-command="ctest --test-dir build" \
  --test-result-dir=./build/test-results \
  --scope=commit \
  --limit=50 \
  --exclude="*/third_party/*" \
  --exclude="*/test/*"
```

### Sample Output

After mutant generation, Sentinel prints a **Mutant Population Report** summarising what was generated:

```
==============================================================
               Mutant Population Report
==============================================================
File                                                  Mutants
--------------------------------------------------------------
src/foo.cpp                                                45
src/bar.cpp                                                30
--------------------------------------------------------------
TOTAL                                                      75
==============================================================

Operator                                              Mutants
--------------------------------------------------------------
AOR                                                        42
LCR                                                        33
--------------------------------------------------------------
TOTAL                                                      75
==============================================================
Generator : uniform  |  Seed: 3721894056
Analyzed  : 320 source lines across 2 files
Selected  : 75 out of 320 candidates
==============================================================
```

After all mutants are evaluated, a **Mutation Coverage Report** is printed to stdout.

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
| `--verbose` | Enable verbose output (INFO level and above) | |
| `--silent` | Suppress build/test log output; status line still shows progress | |
| `--debug` | Enable debug output (all log levels) | |
| `-f, --force` | Skip all prompts and start fresh, overwriting any previous state | |
| `-w, --workspace=PATH` | Directory for all run artifacts | `./.sentinel` |
| `-o, --output-dir=PATH` | Directory to write HTML/XML reports | |

#### Run options

| Option | Description | Default |
|--------|-------------|---------|
| `--config=PATH` | YAML config file path. When the config is in a different directory, sentinel changes to that location before running; a pre-run warning is shown. | `sentinel.yaml` (auto-detected) |
| `--init` | Write a `sentinel.yaml` config template and exit | |
| `--dry-run` | Build, test, and generate mutants, then print a readiness summary and exit without evaluating any mutant. The workspace is preserved so that the next `sentinel` invocation (without `--dry-run`) resumes directly at the evaluation phase. Combine with `--verbose` to also list every generated mutant. | |
| `--no-statusline` | Disable the live terminal status line | |
| `--threshold=PCT` | Fail with exit code 3 if the mutation score is below this percentage (0–100). When the run completes, a one-line score summary is always printed to stderr. If no evaluable mutants exist, the threshold is not applied. | disabled |

#### Build & test options

| Option | Description | Default |
|--------|-------------|---------|
| `--source-dir=PATH` | Root of the source tree to test | `.` |
| `--build-command=CMD` | Shell command to build the project | **required** |
| `--compiledb-dir=PATH` | Directory containing `compile_commands.json` | `.` |
| `--test-command=CMD` | Shell command to run tests | **required** |
| `--test-result-dir=PATH` | Directory where the test command writes result files | **required** |
| `--test-result-ext=EXT` | File extension of test result files (repeatable) | `xml` |
| `--timeout=SEC` | Test time limit; `0` = no limit (triggers pre-run warning), `auto` = 2× baseline run time | `auto` |
| `--kill-after=SEC` | Seconds after timeout before sending SIGKILL (0 = disabled) | `60` |

#### Mutation options

| Option | Description | Default |
|--------|-------------|---------|
| `-s, --scope=SCOPE` | `commit` (changed lines only) or `all` (entire codebase) | `all` |
| `-t, --extension=EXT` | Source file extensions to mutate (repeatable) | `cxx cpp cc c c++ cu` |
| `-p, --pattern=PATTERN` | Git pathspec patterns to constrain the mutation scope (repeatable). Matched against repository-relative paths. Absolute paths trigger a pre-run warning. | |
| `-e, --exclude=PATTERN` | fnmatch patterns matched against canonical absolute file paths (repeatable). Patterns without a leading `*` or ending in `/` trigger a pre-run warning. | |
| `-l, --limit=N` | Maximum number of mutants to generate; `0` = evaluate all candidates (triggers pre-run warning) | `0` |
| `--partition=N/TOTAL` | Evaluate only the N-th contiguous slice of the full mutant list out of TOTAL partitions (1-based, e.g., `--partition=2/5`). Requires `--seed` to be explicitly set so every partition instance generates an identical mutant list. The union of all partition results equals a single non-partitioned run. When used with `--limit`, the limit is applied before slicing — setting `--limit` smaller than TOTAL triggers a pre-run warning. | disabled |
| `--generator=TYPE` | Mutant selection strategy: `uniform`, `random`, or `weighted` | `uniform` |
| `--seed=N` | Random seed for mutant selection | random |
| `--operator=OP` | Mutation operators to apply (repeatable; defaults to all) | all |
| `--coverage=FILE` | lcov coverage info file; limits mutation to covered lines (repeatable) | |

---

## Configuration File

Sentinel supports a YAML configuration file (`sentinel.yaml`) as an alternative to passing all options on the command line.

When invoked, Sentinel looks for `sentinel.yaml` in the current working directory automatically. Use `--config` to specify a different file, or run `sentinel --init` to generate a template.

All relative paths in `sentinel.yaml` are resolved relative to the file's own location. Sentinel also changes to that directory before executing build and test commands, so paths in the config and shell commands share the same base.

**Precedence:**
```
CLI argument  >  sentinel.yaml  >  built-in default
```

### Minimal Example

```yaml
# sentinel.yaml
version: 1
source-dir: ./src
compiledb-dir: ./build
build-command: cmake -B build && cmake --build build
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
#
# Uncomment and edit the options you need.
# CLI arguments always take priority over values in this file.

# Directory for output reports (default: none)
# output-dir: ./sentinel_output

# Workspace directory for all sentinel run artifacts (default: ./.sentinel)
# workspace: ./.sentinel

# --- Run options ---

# Fail with exit code 3 if mutation score is below this percentage 0–100 (default: disabled)
# threshold: 80

# Evaluate only the N-th slice of all mutants out of TOTAL partitions (e.g. 2/4); requires --seed
# partition: 1/1

# --- Build & test options ---

# Source root directory (default: .)
# source-dir: .

# Shell command to build the source
# build-command: make

# Path to directory containing compile_commands.json (default: .)
# compiledb-dir: .

# Shell command to execute tests
# test-command: make test

# Path to the test report directory
# test-result-dir: ./test-results

# File extension of the test report (default: xml)
# test-result-ext:
#   - xml

# Test time limit in seconds (default: auto — 2x baseline run time; 0 = no limit)
# timeout: auto

# Seconds to wait after timeout before sending SIGKILL (default: 60; 0 = disabled)
# kill-after: 60

# --- Mutation options ---

# Diff scope: 'commit' (changed lines only) or 'all' (entire codebase) (default: all)
# scope: all

# Source file extensions to mutate (default: cxx cpp cc c c++ cu)
# extension:
#   - cpp
#   - cxx
#   - cc
#   - c
#   - c++
#   - cu

# Paths or glob patterns to constrain the diff (default: none — entire source)
# pattern: []

# Paths excluded from mutation; fnmatch-style patterns (default: none)
# exclude: []

# Maximum number of mutants to generate (default: 0 = unlimited)
# limit: 0

# Mutant generator type: 'uniform', 'random', or 'weighted' (default: uniform)
# generator: uniform

# Random seed for mutant selection (default: auto — picked randomly)
# seed: auto

# Mutation operators to use; omit to use all operators (default: all)
# Valid values: AOR, BOR, LCR, ROR, SDL, SOR, UOI
# operator:
#   - AOR
#   - BOR
#   - LCR
#   - ROR
#   - SDL
#   - SOR
#   - UOI

# lcov-format coverage result files; limits mutation to covered lines only (default: none)
# coverage: []
```

---

## Coverage-Guided Mutation

The `--coverage` option accepts an lcov coverage info file and restricts mutation to source lines that are actually executed by the test suite. This reduces the number of mutants and focuses testing on covered code.

After running the tests with coverage instrumentation enabled, generate the coverage info file with lcov:

```bash
lcov --capture --directory . --output-file coverage.info

# (Optional) Remove system and third-party paths
lcov --remove coverage.info '/usr/*' '*/third_party/*' --output-file coverage.info
```

Then pass it to Sentinel:

```bash
sentinel --coverage=coverage.info ...
```

The `--coverage` option can be repeated to merge multiple coverage files.

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

## Exit Codes

| Code | Meaning |
|------|---------|
| `0` | Success |
| `1` | Runtime error (build failure, I/O error, invalid option value, etc.) |
| `2` | CLI argument error |
| `3` | Mutation score is below the `--threshold` value |

---

## Supported Test Result Formats

Sentinel parses JUnit-style XML test results. The following formats are supported:

| Format | Produced by |
|--------|-------------|
| GoogleTest XML | [GoogleTest](https://github.com/google/googletest) |
| QTest XML | [QTest](https://doc.qt.io/qt-6/qtest-overview.html) |
| CTest XML | [CTest](https://cmake.org/cmake/help/latest/manual/ctest.1.html) |

The test runner must be configured to write results to the directory specified by `--test-result-dir`.

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
