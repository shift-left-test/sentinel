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
pattern:
  - "!*/third_party/*"
  - "!*/test/*"
```

```bash
sentinel --limit=50
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
  --pattern="!*/third_party/*" \
  --pattern="!*/test/*"
```

### Sample Output

### Understanding the Output

Sentinel produces output at three stages: generation, evaluation, and the final report.

#### 1. Mutant Population Report (Generation)

After mutant generation, Sentinel prints a summary of what was generated:

```
==============================================================
               Mutant Population Report
==============================================================
File                                            Mutants  Lines
--------------------------------------------------------------
src/foo.cpp                                          45     180
src/bar.cpp                                          30     140
--------------------------------------------------------------
TOTAL                                                75     320
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

| Field | Description |
|-------|-------------|
| **File / Mutants / Lines** | Per-file breakdown of generated mutants and analyzed source lines |
| **Operator / Mutants** | Per-operator breakdown (AOR, LCR, ROR, etc.) |
| **Generator** | Mutant selection strategy used (`uniform`, `random`, or `weighted`) |
| **Seed** | Random seed — use this value with `--seed` to reproduce the same mutant set |
| **Analyzed** | Total source lines scanned across all target files |
| **Selected** | How many mutants were selected out of all possible candidates |

#### 2. Per-Mutant Evaluation Results

During evaluation, each mutant result is printed as it completes:

```
Evaluating 75 mutants...
  [  1/75] ✗ KILLED        AOR  src/foo.cpp:42 (+)  [1s/1s]
           ← CalculatorTest.AddOverflow, CalculatorTest.AddNegative
  [  2/75] ✓ SURVIVED      ROR  src/foo.cpp:58 (<)  [1s/1s]
  [  3/75] ⚠ BUILD_FAILURE SDL  src/bar.cpp:15 (DELETE)  [0s/0s]
           ↪ .sentinel/mutants/3/build.log
  [  4/75] ⚠ TIMEOUT       AOR  src/bar.cpp:27 (-)  [1s/10s]
           ↪ .sentinel/mutants/4/test.log
```

Each line contains:

| Part | Description |
|------|-------------|
| `[1/75]` | Progress counter (current / total) |
| `✗` / `✓` / `⚠` | Result icon — ✗ killed, ✓ survived, ⚠ skipped (build failure, timeout, or runtime error) |
| `KILLED` | Mutation state (see table below) |
| `AOR` | Mutation operator that was applied |
| `src/foo.cpp:42` | File and line number of the mutation |
| `(+)` | The original token that was replaced (or `DELETE` for statement deletion) |
| `[build/test]` | Time spent building and testing |
| `←` | Killing test names (shown only for killed mutants; up to 2, with "+N more" if more exist) |
| `↪` | Log file path (shown for build failures, timeouts, and runtime errors) |

#### 3. Mutation Score Report (Final)

After all mutants are evaluated, Sentinel prints the final summary to stdout:

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                          Mutation Score Report
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  File                                              Killed  Survived  Total  Score
──────────────────────────────────────────────────────────────────────────────────
  src/foo.cpp                                           12         3     15  80.0%
  src/bar.cpp                                            8         2     10  80.0%
──────────────────────────────────────────────────────────────────────────────────
  TOTAL                                                 20         5     25  80.0%
──────────────────────────────────────────────────────────────────────────────────
  Skipped: 2 build failures, 1 timeout
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

| Column | Description |
|--------|-------------|
| **Killed** | Mutants detected by the test suite (a test failed) |
| **Survived** | Mutants *not* detected (all tests still passed) — indicates a gap in test coverage |
| **Total** | Killed + Survived (excludes skipped mutants) |
| **Score** | Killed / Total as a percentage |

When **Total** is 0 for a file (all mutants were skipped), the score is displayed as `-%`.

The **Skipped** line lists counts of build failures, timeouts, and runtime errors. It is omitted when there are no skipped mutants.

A final one-line summary is always written to stderr:

```
Mutation testing complete – 25 mutants, score: 80.0% (threshold: 70.0% ✓)
```

#### Mutation States

Every mutant evaluation results in one of five states:

| State | Icon | Meaning | What to do |
|-------|------|---------|------------|
| **Killed** | ✗ | A test failed on the mutant — the test suite detected the fault. | Nothing; this is the desired outcome. |
| **Survived** | ✓ | All tests passed despite the mutation — the fault went undetected. | Write a test that covers the mutated code path. |
| **Build Failure** | ⚠ | The mutant caused a compilation error. | Usually harmless; the compiler caught the fault. Skipped from the score. |
| **Timeout** | ⚠ | Tests took longer than the time limit. | May indicate an infinite loop caused by the mutation. Skipped from the score. |
| **Runtime Error** | ⚠ | Tests crashed or produced an abnormal exit. | May indicate a null dereference, assertion failure, or segfault caused by the mutation. Skipped from the score. |

#### Mutation Score

The mutation score measures how well the test suite detects faults:

```
Mutation Score = Killed / (Killed + Survived) × 100
```

Build failures, timeouts, and runtime errors are **excluded** from the score (shown as "Skipped" in the report). Only mutants that compiled and ran to completion count toward the score.

#### Terminal Status Line

When stdout is a TTY, Sentinel displays a live status line at the bottom of the terminal:

```
 EVALUATION  │  [42/75] (56%)  │  ✗ 30 ✓  8 ⚠  4  │  78.9%  │  00:42:17
```

| Symbol | Meaning |
|--------|---------|
| **✗** | Killed |
| **✓** | Survived |
| **⚠** | Abnormal (Build Failure + Timeout + Runtime Error) |

The status line is automatically suppressed when output is piped or redirected.

### Resume

If Sentinel is interrupted, rerun it with the same `--workspace` path. It will detect the previous run and prompt to resume.

---

### Option Reference

#### Run options

| Option | Description | Default |
|--------|-------------|---------|
| `--config=PATH` | YAML config file path. When the config is in a different directory, sentinel changes to that location before running; a pre-run warning is shown. | `sentinel.yaml` (auto-detected) |
| `--workspace=PATH` | Directory for all run artifacts | `./.sentinel` |
| `-c, --clean` | Clear workspace and start a fresh run instead of resuming | |
| `-o, --output-dir=PATH` | Directory to write HTML/XML reports | |
| `-n, --dry-run` | Build, test, and generate mutants, then exit without evaluating any mutant. The workspace is preserved so that the next `sentinel` invocation (without `--dry-run`) resumes directly at the evaluation phase. | |
| `-v, --verbose` | Show build/test subprocess output and enable verbose logging to stderr | |

#### Setup options

| Option | Description | Default |
|--------|-------------|---------|
| `--init` | Write a `sentinel.yaml` config template and exit | |
| `--force` | Overwrite existing files (used with `--init`) | |

#### Build & test options

| Option | Description | Default |
|--------|-------------|---------|
| `--source-dir=PATH` | Root of the source tree to test | `.` |
| `--build-command=CMD` | Shell command to build the project | **required** |
| `--compiledb-dir=PATH` | Directory containing `compile_commands.json` | `.` |
| `--test-command=CMD` | Shell command to run tests | **required** |
| `--test-result-dir=PATH` | Directory where the test command writes result files | **required** |
| `--test-result-ext=EXT` | File extension of test result files (repeatable) | `xml` |
| `--timeout=SEC` | Test time limit in seconds; `0` = no limit (triggers pre-run warning) | 1.5× baseline |

#### Mutation options

| Option | Description | Default |
|--------|-------------|---------|
| `-s, --scope=SCOPE` | `commit` (changed lines only) or `all` (entire codebase) | `all` |
| `-p, --pattern=PATTERN` | Glob patterns to constrain the mutation scope (repeatable). Matched against repository-relative paths. Prefix with `!` to exclude matching files. Absolute paths trigger a pre-run warning. | |
| `--extension=EXT` | Source file extensions to mutate (repeatable) | `cxx cpp cc c c++ cu` |
| `--generator=TYPE` | Mutant selection strategy: `uniform`, `random`, or `weighted` | `uniform` |
| `--seed=N` | Random seed for mutant selection | random |
| `--operator=OP` | Mutation operators to apply (repeatable; defaults to all) | all |

#### Advanced options

| Option | Description | Default |
|--------|-------------|---------|
| `-l, --limit=N` | Maximum number of mutants to generate; `0` = unlimited | `0` |
| `--coverage=FILE` | lcov coverage info file; limits mutation to covered lines (repeatable) | |
| `--partition=N/TOTAL` | Evaluate only the N-th contiguous slice of the full mutant list out of TOTAL partitions (1-based, e.g., `--partition=2/5`). It is recommended to set `--seed` explicitly so every partition instance generates an identical mutant list; if omitted, a random seed is used and each run may evaluate a different subset. The union of all partition results equals a single non-partitioned run. Mutant paths are stored relative to `--source-dir`, so workspace directories can be collected from multiple machines and resumed on any machine with the same source tree. When used with `--limit`, the limit is applied before slicing — setting `--limit` smaller than TOTAL triggers a pre-run warning. | disabled |
| `--threshold=PCT` | Fail with exit code 3 if the mutation score is below this percentage (0.0–100.0). When the run completes, a one-line score summary is always printed to stderr. If no evaluable mutants exist, the threshold is not applied. | disabled |

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
pattern:
  - "!*/third_party/*"
  - "!*/test/*"
```

### Full Template

Run `sentinel --init` to write a fully commented template to the current directory, or see the template below.

```yaml
# sentinel.yaml — full configuration template
#
# Uncomment and edit the options you need.
# CLI arguments always take priority over values in this file.

## Config file format version (required)
version: 1

## Directory for output reports (default: none)
# output-dir: ./sentinel_output

# --- Build & test options ---

## Source root directory (default: .)
# source-dir: .

## Shell command to build the source
# build-command: make

## Path to directory containing compile_commands.json (default: .)
# compiledb-dir: .

## Shell command to execute tests
# test-command: make test

## Path to the test report directory
# test-result-dir: ./test-results

## File extension of the test report (default: xml)
# test-result-ext:
#   - xml

## Test time limit in seconds (default: 1.5x original test time; 0 = no limit)
# timeout: 60

# --- Mutation options ---

## Diff scope: 'commit' (changed lines only) or 'all' (entire codebase) (default: all)
# scope: all

## Source file extensions to mutate (default: cxx cpp cc c c++ cu)
# extension:
#   - cpp
#   - cxx
#   - cc
#   - c
#   - c++
#   - cu

## Paths or glob patterns to constrain mutation scope (default: none - entire source)
## Prefix a pattern with ! to exclude matching files (e.g. "!*/test/*")
# pattern: []

## Mutant selection strategy (default: uniform)
##   uniform  - one mutant per operator per source line
##   random   - randomly sampled from all possible mutants
##   weighted - samples more mutants from complex code
# generator: uniform

## Mutation operators to use; omit to use all operators (default: all)
# operator:
#   - AOR  # Arithmetic Operator Replacement  (+, -, *, /)
#   - BOR  # Bitwise Operator Replacement     (&, |, ^)
#   - LCR  # Logical Connector Replacement    (&&, ||)
#   - ROR  # Relational Operator Replacement  (<, >, ==, !=)
#   - SDL  # Statement Deletion
#   - SOR  # Shift Operator Replacement       (<<, >>)
#   - UOI  # Unary Operator Insertion         (-x, !x)

# --- Advanced options ---

## lcov-format coverage result files; limits mutation to covered lines only (default: none)
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
cmake -DCMAKE_TESTING_ENABLED=ON .
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
