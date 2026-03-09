#!/bin/bash
set -e
cmake . -DENABLE_TEST=ON
make doc
make -j
ctest --output-on-failure
make coverage
make package
