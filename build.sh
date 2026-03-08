#!/bin/bash
set -e
cmake -S . -B build -DENABLE_TEST=ON
cmake --build build --target doc
cmake --build build --target all -j
ctest --test-dir build --output-on-failure
cmake --build build --target coverage
cmake --build build --target package
