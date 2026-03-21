# Sentinel Sample Project

A minimal C++ project demonstrating how to use [Sentinel](https://github.com/shift-left-test/sentinel) for mutation testing.

The project implements a simple `Stack` (integer stack) with a GoogleTest suite, pre-configured so you can run mutation testing with a single command.

---

## Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| CMake | 3.14+ | |
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

From the `sample/` directory (the build directory must exist — see Build above):

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
