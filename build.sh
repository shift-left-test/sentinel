#!/bin/bash

cmake . -DENABLE_TEST=ON
make all -j
ctest --output-on-failure
make coverage
