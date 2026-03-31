# Sentinel Sample Project

A C++ calculator project demonstrating how to use [Sentinel](https://github.com/shift-left-test/sentinel) for mutation testing.

The project implements a stack-based expression evaluator using the **Shunting-yard algorithm** (Dijkstra). It parses and evaluates mathematical expressions like `1 + 2 * (1 + 2)` with correct operator precedence and parentheses support.

---

## Components

| Class | Description |
|-------|-------------|
| `Stack` | Integer stack (push, pop, peek) |
| `Tokenizer` | Breaks expression strings into tokens (numbers, operators, parentheses) |
| `Calculator` | Evaluates expressions using Shunting-yard with two stacks |

### Supported Operations

- Arithmetic: `+`, `-`, `*`, `/`
- Parentheses: `(`, `)`
- Negative numbers: `-5`, `3 * -2`

### Example

```bash
./build/calc "1 + 2 * (1 + 2)"
# Output: 7

./build/calc "100 / (2 * 5) + 3 * (4 - 1)"
# Output: 19
```

---

## Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| CMake | 3.13+ | |
| C++ compiler | C++17 support | GCC or Clang |
| sentinel | any | Must be on `PATH` |
| gcovr | any | For coverage report (optional) |
| Internet access | — | GoogleTest is downloaded via FetchContent on first build |

---

## Build

From the `sample/` directory:

```bash
cmake -B build
cmake --build build
```

Verify the tests pass:

```bash
cd build && ctest --output-on-failure
```

### Build with Coverage

```bash
cmake -DCMAKE_TESTING_ENABLED=ON -B build
cmake --build build
cd build && ctest --output-on-failure
```

After running the tests, `.gcda` / `.gcno` files are generated in the build tree. Use `gcovr` to produce an HTML report:

```bash
gcovr --html-details coverage.html -r ../src .
```

---

## Run Mutation Testing

From the `sample/` directory:

```bash
sentinel
```

Sentinel reads `sentinel.yaml` automatically. It will:

1. Build the project and run the test suite once (baseline)
2. Generate up to 30 mutants from `src/` (adjust `limit` in `sentinel.yaml` to change this, or remove it to test all mutants)
3. Apply each mutant, rebuild, and check whether the tests catch it
4. Write an HTML report to `./sentinel_output/`

### View the Report

```bash
xdg-open sentinel_output/index.html   # Linux
open sentinel_output/index.html        # macOS
```

---

## What to Expect

Sentinel produces output at three stages. Here is what you will see when running it on this sample project.

### 1. Mutant Population Report

After generation, Sentinel shows what was created:

```
==============================================================
               Mutant Population Report
==============================================================
File                                            Mutants  Lines
--------------------------------------------------------------
src/Calculator.cpp                                   12     85
src/Stack.cpp                                         8     52
src/Tokenizer.cpp                                    10     63
--------------------------------------------------------------
TOTAL                                                30    200
==============================================================
Generator : uniform  |  Seed: 1234567890
Analyzed  : 200 source lines across 3 files
Selected  : 30 out of 200 candidates
==============================================================
```

This tells you how many mutants were generated per file, which generator strategy was used, and the random seed (use `--seed` to reproduce the same mutant set).

### 2. Per-Mutant Evaluation

Each mutant is evaluated one by one, with results printed in real time:

```
Evaluating 30 mutants...
  [  1/30] ✗ KILLED        AOR  src/Calculator.cpp:42 (+)  [1s/1s]
           ← CalculatorTest.AddTwoNumbers
  [  2/30] ✓ SURVIVED      ROR  src/Stack.cpp:18 (<)  [0s/1s]
  [  3/30] ⚠ BUILD_FAILURE SDL  src/Tokenizer.cpp:31 (DELETE)  [0s/0s]
           ↪ .sentinel/mutants/3/build.log
```

- **✗ KILLED** — a test caught the mutation (the `←` line shows which test)
- **✓ SURVIVED** — no test caught it; this is a test coverage gap
- **⚠ BUILD_FAILURE / TIMEOUT / RUNTIME_ERROR** — skipped from the score (the `↪` line points to the log file)

### 3. Mutation Coverage Report

After all evaluations, the final summary:

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                         Mutation Coverage Report
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  File                                              Killed  Survived  Total  Score
──────────────────────────────────────────────────────────────────────────────────
  src/Calculator.cpp                                     8         2     10  80.0%
  src/Stack.cpp                                          5         1      6  83.3%
  src/Tokenizer.cpp                                      9         3     12  75.0%
──────────────────────────────────────────────────────────────────────────────────
  TOTAL                                                 22         6     28  78.6%
──────────────────────────────────────────────────────────────────────────────────
  Skipped: 2 build failures
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

The **mutation score** (Killed / Total) tells you how effective your tests are at detecting faults. Only Killed and Survived mutants count; build failures, timeouts, and runtime errors are listed under "Skipped".

| State | Icon | Meaning |
|-------|------|---------|
| **Killed** | ✗ | A test failed on the mutant — desired outcome |
| **Survived** | ✓ | All tests passed despite the mutation — test gap |
| **Build Failure** | ⚠ | Mutant did not compile — excluded from score |
| **Timeout** | ⚠ | Tests exceeded the time limit — excluded from score |
| **Runtime Error** | ⚠ | Tests crashed or exited abnormally — excluded from score |

### Improving the Score

Open the HTML report (`sentinel_output/index.html`) to see exactly which mutations survived. Each survived mutant shows the file, line, and the change that was made. Use this to write a targeted test case, then re-run Sentinel to confirm the score improves.
