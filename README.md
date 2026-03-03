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
  - [Workflow Overview](#workflow-overview)
  - [Command Reference](#command-reference)
- [Mutation Operators](#mutation-operators)
- [Supported Test Runners](#supported-test-runners)
- [Development](#development)
- [Licenses](#licenses)

---

## About

Mutation testing is a technique to evaluate and improve the quality of a test suite. It works by injecting small, deliberate faults (mutants) into the source code and checking whether the existing tests can detect them.

**Benefits for developers:**
- Measure the effectiveness of a test suite using the *mutation score*
- Identify weak spots in tests and get guidance on writing better test cases

Sentinel makes mutation testing practical for C/C++ projects by:
- Supporting diverse configuration options
- Limiting mutation scope to committed changes (to reduce cost)
- Automatically generating HTML/XML reports

> For more background, see [Mutation testing on Wikipedia](https://en.wikipedia.org/wiki/Mutation_testing).

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
sentinel run /path/to/your/project \
  --build-command="make" \
  --test-command="make test"
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

### Workflow Overview

Sentinel provides individual commands for each phase of mutation testing. You can either run them step by step or use the `run` command to execute the entire pipeline at once.

```
populate → mutate → evaluate → report
    ↑                               ↑
find mutants                 generate report

          OR use: sentinel run
          (runs all steps automatically)
```

| Step       | Command    | Description                                      |
|------------|------------|--------------------------------------------------|
| 1. Find    | `populate` | Identify mutable locations in the source code    |
| 2. Apply   | `mutate`   | Apply a single mutant to the source              |
| 3. Check   | `evaluate` | Compare test results with and without the mutant |
| 4. Report  | `report`   | Generate an HTML/XML report from results         |
| All-in-one | `run`      | Execute all steps in standalone mode             |

### Command Reference

#### `run` — All-in-one mutation test

```bash
sentinel run [SOURCE_ROOT_PATH] {OPTIONS}
```

| Option | Description | Default |
|--------|-------------|---------|
| `SOURCE_ROOT_PATH` | Source root directory | |
| `-v, --verbose` | Enable verbose output | |
| `-w, --work-dir=PATH` | Temporary working directory | `./sentinel_tmp` |
| `-o, --output-dir=PATH` | Directory for output files | |
| `-b, --build-dir=PATH` | Directory containing `compile_commands.json` | `.` |
| `--compiledb=PATH` | Path to directory containing `compile_commands.json` | |
| `--build-command=CMD` | Shell command to build the source | |
| `--test-command=CMD` | Shell command to run tests | |
| `--test-result-dir=PATH` | Directory for test output files | |
| `--test-result-extension=EXT` | File extensions of test output | |
| `-t, --extension=EXT` | Source file extensions to mutate | |
| `-e, --exclude=PATTERN` | Exclude paths matching fnmatch-style patterns | |
| `--coverage=COV.INFO` | lcov-format coverage file | |
| `-s, --scope=SCOPE` | Diff scope: `commit` or `all` | `all` |
| `-l, --limit=COUNT` | Maximum number of mutants to generate | `10` |
| `--generator=TYPE` | Mutant generator: `uniform`, `random`, or `weighted` | `uniform` |
| `--seed=SEED` | Random seed | random |
| `--timeout=SEC` | Time limit for test command; `0` = no limit, `auto` = auto-detect | `auto` |
| `--kill-after=SEC` | Send SIGKILL this many seconds after timeout; `0` = disabled | `60` |

**Example:**

```bash
sentinel run ./my-project \
  --build-command="cmake --build build" \
  --test-command="ctest --test-dir build" \
  --scope=commit \
  --limit=50 \
  --exclude="*/test/*" "*/third_party/*"
```

---

#### `populate` — Find mutable locations

Scans the source tree and generates a list of mutable locations.

```bash
sentinel populate [SOURCE_ROOT_PATH] {OPTIONS}
```

| Option | Description | Default |
|--------|-------------|---------|
| `SOURCE_ROOT_PATH` | Source root directory | |
| `-v, --verbose` | Enable verbose output | |
| `-w, --work-dir=PATH` | Temporary working directory | `./sentinel_tmp` |
| `-o, --output-dir=PATH` | Directory for output files | `.` |
| `-b, --build-dir=PATH` | Directory containing `compile_commands.json` | `.` |
| `--compiledb=PATH` | Path to directory containing `compile_commands.json` | |
| `-s, --scope=SCOPE` | Diff scope: `commit` or `all` | `all` |
| `-t, --extension=EXT` | Source file extensions to mutate | |
| `-e, --exclude=PATTERN` | Exclude paths matching fnmatch-style patterns | |
| `-l, --limit=COUNT` | Maximum number of mutants to generate | `10` |
| `--mutants-file-name=PATH` | Output file name for mutant list | `mutables.db` |
| `--generator=TYPE` | Mutant generator: `uniform`, `random`, or `weighted` | `uniform` |
| `--seed=SEED` | Random seed | `1942447250` |

---

#### `mutate` — Apply a mutant

Applies a single mutant to the source code. The original file is backed up in `work-dir`.

```bash
sentinel mutate [SOURCE_ROOT_PATH] {OPTIONS}
```

| Option | Description | Default |
|--------|-------------|---------|
| `SOURCE_ROOT_PATH` | Source root directory | |
| `-v, --verbose` | Enable verbose output | |
| `-w, --work-dir=PATH` | Temporary working directory | `./sentinel_tmp` |
| `-o, --output-dir=PATH` | Directory for output files | `.` |
| `-m, --mutant=MUTANT` | Mutant string to apply | |

---

#### `evaluate` — Compare test results

Compares test results before and after a mutant is applied to determine if the mutant was killed.

```bash
sentinel evaluate [SOURCE_ROOT_PATH] {OPTIONS}
```

| Option | Description | Default |
|--------|-------------|---------|
| `SOURCE_ROOT_PATH` | Source root directory | |
| `-v, --verbose` | Enable verbose output | |
| `-w, --work-dir=PATH` | Temporary working directory | `./sentinel_tmp` |
| `-o, --output-dir=PATH` | Directory for output files | `.` |
| `-m, --mutant=MUTANT` | Mutant string | |
| `-e, --expected=PATH` | Expected (baseline) test result directory | |
| `-a, --actual=PATH` | Actual (mutated) test result directory | |
| `--evaluation-file=PATH` | Output filename for evaluation results | `EvaluationResults` |
| `--test-state=STATE` | Test state to evaluate: `success`, `build_failure`, `timeout`, `uncovered` | `success` |

---

#### `report` — Generate a report

Creates an HTML or XML mutation test report from the evaluation results.

```bash
sentinel report [SOURCE_ROOT_PATH] {OPTIONS}
```

| Option | Description | Default |
|--------|-------------|---------|
| `SOURCE_ROOT_PATH` | Source root directory | |
| `-v, --verbose` | Enable verbose output | |
| `-w, --work-dir=PATH` | Temporary working directory | `./sentinel_tmp` |
| `-o, --output-dir=PATH` | Directory for output files (omit to skip file generation) | |
| `--evaluation-file=PATH` | Mutation test result file to read | |

---

## Mutation Operators

Sentinel supports the following mutation operators:

| Operator | Name | Description | Example |
|----------|------|-------------|---------|
| **AOR** | Arithmetic Operator Replacement | Replaces arithmetic operators | `a + b` → `a - b` |
| **BOR** | Bitwise Operator Replacement | Replaces bitwise operators | `a & b` → `a \| b` |
| **LCR** | Logical Connector Replacement | Replaces logical operators | `a && b` → `a \|\| b` |
| **ROR** | Relational Operator Replacement | Replaces relational operators | `a < b` → `a > b` |
| **SDL** | Statement Deletion | Deletes a statement | `return x;` → *(deleted)* |
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
