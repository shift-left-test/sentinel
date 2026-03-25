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

- **Killed mutant** — the test suite caught the fault. Good.
- **Survived mutant** — the test suite missed it. This is a gap in test coverage.

After the run, try adding a test case that kills a survived mutant, then re-run sentinel to confirm the score improves.
