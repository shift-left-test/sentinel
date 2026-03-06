#!/bin/bash
set -e
cmake -S . -B out -DENABLE_TEST=ON
cmake --build out --target doc
cmake --build out --target all -j
ctest --test-dir out --output-on-failure
cmake --build out --target coverage
cmake --build out --target package
