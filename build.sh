#!/bin/bash

cmake . -DENABLE_TEST=ON || exit 1
make doc || exit 1
make all -j || exit 1
ctest --output-on-failure || exit 1
make coverage
