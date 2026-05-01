# Sentinel

**Sentinel** is a mutation testing tool based on LLVM/Clang for C/C++ projects.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](CMakeLists.txt)

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
- Integrating with git to limit mutations to recent changes (`--from=HEAD~1`, `--uncommitted`)
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
sudo apt-get install ./sentinel_1.0.0-1~ubuntu20.04+llvm14_amd64.deb
```

The exact deb filename varies with the Ubuntu release and LLVM major version
used at build time. Use a glob (e.g. `./sentinel_*.deb`) if you do not want
to spell it out.

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
sudo apt-get install ./sentinel_*.deb
```

The generated deb encodes the Ubuntu release and LLVM major version in its
filename (e.g. `sentinel_1.0.0-1~ubuntu20.04+llvm14_amd64.deb`) and declares
a runtime dependency on the matching `clang-N` package, which `apt` will
install automatically.

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
  --from=HEAD~1 \
  --uncommitted \
  --limit=50 \
  --pattern="!*/third_party/*" \
  --pattern="!*/test/*"
```

### Sample Output

### Understanding the Output

Sentinel produces output at three stages: generation, evaluation, and the final report.

#### 1. Mutant Generation Summary

After mutant generation, Sentinel prints a summary of what was generated:

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                       Mutant Generation Summary
  Target:     all sources (2 files, 320 lines)
  Generator:  uniform (seed: 3721894056)
  Mutants:    75
────────────────────────────────────────────────────────────────────────────────────
  File                                                    Mutants    Lines
────────────────────────────────────────────────────────────────────────────────────
  src/foo.cpp                                                  45      180
  src/bar.cpp                                                  30      140
────────────────────────────────────────────────────────────────────────────────────
  Operator
────────────────────────────────────────────────────────────────────────────────────
  AOR (Arithmetic Operator Replacement)                        42
  LCR (Logical Connector Replacement)                          33
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

| Field | Description |
|-------|-------------|
| **Target** | Diff base and number of target files and total source lines scanned |
| **Generator** | Mutant selection strategy used and random seed (use with `--seed` to reproduce) |
| **Mutants** | Number of mutants selected (with limit and partition info if applicable) |
| **File / Mutants / Lines** | Per-file breakdown of generated mutants and analyzed source lines |
| **Operator** | Per-operator breakdown with full operator name |

#### 2. Per-Mutant Evaluation Results

During evaluation, each mutant result is printed as it completes:

```
Evaluating 75 mutants...
  [  1/75] ✗ KILLED        AOR  src/foo.cpp:42:5 (+)  [1s/1s]
           ← CalculatorTest.AddOverflow, CalculatorTest.AddNegative
  [  2/75] ✓ SURVIVED      ROR  src/foo.cpp:58:12 (<)  [1s/1s]
  [  3/75] ⚠ BUILD_FAILURE SDL  src/bar.cpp:15:3 (DELETE)  [0s/0s]
           ↪ .sentinel_workspace/mutants/3/build.log
  [  4/75] ⚠ TIMEOUT       AOR  src/bar.cpp:27:8 (-)  [1s/10s]
           ↪ .sentinel_workspace/mutants/4/test.log
```

Each line contains:

| Part | Description |
|------|-------------|
| `[1/75]` | Progress counter (current / total) |
| `✗` / `✓` / `⚠` | Result icon — ✗ killed, ✓ survived, ⚠ skipped (build failure, timeout, or runtime error) |
| `KILLED` | Mutation state (see table below) |
| `AOR` | Mutation operator that was applied |
| `src/foo.cpp:42:5` | File, line, and column of the mutation |
| `(+)` | The original token that was replaced (or `DELETE` for statement deletion) |
| `[build/test]` | Time spent building and testing |
| `←` | Killing test names (shown only for killed mutants; up to 2, with "+N more" if more exist) |
| `↪` | Log file path (shown for build failures, timeouts, and runtime errors) |

#### 3. Mutation Score Report (Final)

After all mutants are evaluated, Sentinel prints the final summary to stdout:

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                          Mutation Score Report
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  File                                       Killed  Survived   Total   Score
────────────────────────────────────────────────────────────────────────────────────
  src/foo.cpp                                    12         3      15   80.0%
  src/bar.cpp                                     8         2      10   80.0%
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  TOTAL                                          20         5      25   80.0%
────────────────────────────────────────────────────────────────────────────────────
  Skipped: 2 build failures, 1 timeout
────────────────────────────────────────────────────────────────────────────────────
  Duration:          100%      42m 17s  [12m 30s/29m 47s]
    Killed            72%      30m 25s  [ 8m 30s/21m 55s]
    Survived          20%       8m 27s  [ 3m 15s/ 5m 12s]
    Timeout            8%       3m 25s  [ 0m 45s/ 2m 40s]
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

| Column | Description |
|--------|-------------|
| **Killed** | Mutants detected by the test suite (a test failed) |
| **Survived** | Mutants *not* detected (all tests still passed) — indicates a gap in test coverage |
| **Total** | Killed + Survived (excludes skipped mutants) |
| **Score** | Killed / Total as a percentage |

When **Total** is 0 for a file (all mutants were skipped), the score is displayed as `-%`.

The **Skipped** line lists counts of build failures, timeouts, and runtime errors. It is omitted when there are no skipped mutants.

The **Duration** section shows total wall time spent on build and test across all evaluated mutants, with a per-state breakdown sorted by time. The `[build/test]` suffix shows the build and test time components separately. This section is omitted when no timing data is available.

A final one-line summary is always written to stderr:

```
Mutation testing complete — 80.0% ✓ (threshold: 70.0%)
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
| `--workspace=PATH` | Directory for all run artifacts | `./.sentinel_workspace` |
| `-c, --clean` | Clear workspace and start a fresh run instead of resuming | |
| `-o, --output-dir=PATH` | Directory to write HTML/XML reports | |
| `-n, --dry-run` | Build, test, and generate mutants, then exit without evaluating any mutant. The workspace is preserved so that the next `sentinel` invocation (without `--dry-run`) resumes directly at the evaluation phase. | |
| `-v, --verbose` | Show build/test subprocess output and enable verbose logging to stderr | |

#### Setup options

| Option | Description | Default |
|--------|-------------|---------|
| `--init` | Write a `sentinel.yaml` config template and exit | |
| `--force` | Force overwrite of existing files | |

#### Build & test options

| Option | Description | Default |
|--------|-------------|---------|
| `--source-dir=PATH` | Root of the source tree to test | `.` |
| `--build-command=CMD` | Shell command to build the project | **required** |
| `--compiledb-dir=PATH` | Directory containing `compile_commands.json` | `.` |
| `--test-command=CMD` | Shell command to run tests | **required** |
| `--test-result-dir=PATH` | Directory where the test command writes result files | **required** |
| `--timeout=SEC` | Test time limit in seconds; `0` = no limit (triggers pre-run warning) | 1.5× baseline |

#### Mutation options

| Option | Description | Default |
|--------|-------------|---------|
| `--from=REV` | Diff base revision. Mutates lines changed between the merge-base of REV and HEAD (committed changes only). Use with `--uncommitted` to also include local changes. See [Scoping with --from and --uncommitted](#scoping-with---from-and---uncommitted). | entire codebase |
| `--uncommitted` | Include uncommitted changes (staged + unstaged + untracked) in mutation scope. Can be used alone or combined with `--from`. | disabled |
| `-p, --pattern=PATTERN` | Glob patterns to constrain the mutation scope (repeatable). Matched against repository-relative paths. Prefix with `!` to exclude matching files. Absolute paths trigger a pre-run warning. | |
| `--extension=EXT` | Source file extensions to mutate (repeatable) | `cxx cpp cc c c++ cu` |
| `--generator=TYPE` | Mutant selection strategy: `uniform`, `random`, or `weighted` | `uniform` |
| `--mutants-per-line=N` | Maximum number of mutants per source line; `0` = unlimited | `1` |
| `--seed=N` | Random seed for mutant selection | random |
| `--operator=OP` | Mutation operators to apply (repeatable; defaults to all) | all |
| `--parallel-parsers=N` | Maximum number of Clang parsers running in parallel during mutant generation; `0` = auto (number of CPU cores). Lower this to reduce peak memory usage. | `0` |

#### Advanced options

| Option | Description | Default |
|--------|-------------|---------|
| `-l, --limit=N` | Maximum number of mutants to generate; `0` = unlimited | `0` |
| `--lcov-tracefile=FILE` | skip evaluation for uncovered mutants (repeatable). The reports and status line show the uncovered subset of SURVIVED separately. | |
| `--restrict` | Restrict mutant generation to lines covered by `--lcov-tracefile`. Without this flag, uncovered lines still produce mutants but their evaluation is skipped (kept in the report as SURVIVED\*). Requires `--lcov-tracefile`. | disabled |
| `--partition=N/TOTAL` | Evaluate only the N-th contiguous slice of the full mutant list out of TOTAL partitions (1-based, e.g., `--partition=2/5`). It is recommended to set `--seed` explicitly so every partition instance generates an identical mutant list; if omitted, a random seed is used and each run may evaluate a different subset. The union of all partition results equals a single non-partitioned run. Mutant paths are stored relative to `--source-dir`, so workspace directories can be collected from multiple machines and resumed on any machine with the same source tree. When used with `--limit`, the limit is applied before slicing — setting `--limit` smaller than TOTAL triggers a pre-run warning. | disabled |
| `--merge-partition PATH` | Merge a partitioned workspace result into the target workspace (repeatable) | |
| `--threshold=PCT` | Fail with exit code 3 if the mutation score is below this percentage (0.0–100.0). When the run completes, a one-line score summary is always printed to stderr. If no evaluable mutants exist, the threshold is not applied. | disabled |

### Scoping with --from and --uncommitted

By default, sentinel mutates the entire codebase. Use `--from` and `--uncommitted` to narrow the scope to only changed code.

#### How --from works

`--from=REV` computes the merge-base between `REV` and `HEAD`, then diffs that merge-base against `HEAD`. Only **committed** changes in that range are included.

```
merge-base(REV, HEAD) ──── ... ──── HEAD
         └── only this range is mutated ──┘
```

Because `--from` uses the merge-base:

| Command | What it covers | Why |
|---------|---------------|-----|
| `--from=HEAD~1` | Last 1 commit | merge-base(HEAD, HEAD~1) = HEAD~1 |
| `--from=HEAD~3` | Last 3 commits | merge-base(HEAD, HEAD~3) = HEAD~3 |
| `--from=main` | All commits since branching from main | merge-base(HEAD, main) = branch point |
| `--from=v1.2.0` | All commits since the tag | merge-base(HEAD, v1.2.0) = tag commit |
| `--from=HEAD` | Nothing (empty range) | merge-base(HEAD, HEAD) = HEAD itself |

> **Note:** `--from=HEAD` produces an empty diff because the merge-base of HEAD with itself is HEAD — there are no commits between HEAD and HEAD. To include the latest commit, use `--from=HEAD~1`.

#### How --uncommitted works

`--uncommitted` includes all local changes not yet committed:
- **Staged** changes (`git add`ed)
- **Unstaged** changes (modified tracked files)
- **Untracked** files (new files not yet `git add`ed, excluding `.gitignore`d files)

#### Combining both options

| Command | Scope |
|---------|-------|
| `sentinel` | Entire codebase |
| `sentinel --uncommitted` | Staged + unstaged + untracked only |
| `sentinel --from=HEAD~1` | Last commit only (committed changes) |
| `sentinel --from=main` | All commits since branching from main |
| `sentinel --from=HEAD~1 --uncommitted` | Last commit + local changes |
| `sentinel --from=main --uncommitted` | All commits since main + local changes |

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
pattern:
  - "!*/third_party/*"
  - "!*/test/*"
```

### Full Template

Run `sentinel --init` to write a fully commented template to the current directory, or see the template below.

```yaml
# sentinel.yaml - full configuration template
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

## Test time limit in seconds (default: 1.5x original test time; 0 = no limit)
# timeout: 60

# --- Mutation options ---

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

## Maximum number of Clang parsers running in parallel during mutant generation
## (default: 0 = auto, number of CPU cores). Lower this to reduce peak memory usage.
# parallel-parsers: 0

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

## lcov tracefiles; skip evaluation for uncovered mutants (default: none)
# lcov-tracefile: []

## When true, restrict mutant generation to lines covered by lcov-tracefile.
## Without this, uncovered mutants are kept in the report as SURVIVED*
## and only their evaluation is skipped. Requires lcov-tracefile to be set.
# restrict: false
```

---

## Coverage-Guided Mutation

The `--lcov-tracefile` option accepts an lcov tracefile and skips build and test evaluation for mutants on lines not covered by the test suite. This reduces evaluation time while still reporting uncovered mutants as survived. The HTML report shows the uncovered count next to the Survived card, donut, and By Operator bar; the XML report marks each entry with an `uncovered="true"` attribute; and the StatusLine summary shows the uncovered subset in parentheses next to the ✓ counter.

After running the tests with coverage instrumentation enabled, generate the coverage info file with lcov:

```bash
lcov --capture --directory . --output-file coverage.info

# (Optional) Remove system and third-party paths
lcov --remove coverage.info '/usr/*' '*/third_party/*' --output-file coverage.info
```

Then pass it to Sentinel:

```bash
sentinel --lcov-tracefile=coverage.info ...
```

The `--lcov-tracefile` option can be repeated to merge multiple tracefiles.

Add `--restrict` to also skip mutant generation for uncovered lines:

```bash
sentinel --lcov-tracefile=coverage.info --restrict ...
```

This excludes uncovered lines from the report entirely instead of keeping them as SURVIVED\*. Use this when uncovered noise (e.g., code that is built into the repo but excluded from the current build configuration) is dragging down the mutation score.

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

Run the full CI build locally:

```bash
./build.sh
```

This runs the same pipeline Jenkins CI uses:
- `cmake -DCMAKE_TESTING_ENABLED=ON .`
- Static analysis (`cppcheck`, `cpplint`)
- Doxygen documentation
- Build (`make -j`)
- Unit tests (`ctest --output-on-failure -j$(nproc)`)
- Code coverage report (`gcovr`)
- Debian package (`make package`)

### Measure coverage only

After running tests, generate a coverage summary with:

```bash
gcovr -s -r . --object-directory .
```

Coverage filters and gcov parse-error handling are configured in `gcovr.cfg`.

---

## Licenses

The project source code is available under the MIT license. See [LICENSE](LICENSE).

Third-party licenses are listed in the [LICENSES](LICENSES) directory.
