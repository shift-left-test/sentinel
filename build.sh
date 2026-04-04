#!/bin/bash
set -e
cmake . -DCMAKE_TESTING_ENABLED=ON
make cppcheck cpplint doc
make -j
find . -name "*.gcda" -delete
ctest --output-on-failure -j$(nproc)
gcovr -s -r . --object-directory .
make package
