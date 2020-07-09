# Mutation Testing Framework

## System Requirements

This program has been developed/tested on Ubuntu 18.04.4 LTS.

- Clang/LLVM 11.0.0
- CMake 3.10.2
- Ninja 1.10.0
- Python 3.6.9

## Installation Instructions

To install [Ninja](https://ninja-build.org/):

```
git clone https://github.com/martine/ninja.git
cd ninja
git checkout release
./bootstrap.py
sudo cp ninja /usr/bin/
```

To install [Clang/LLVM](https://clang.llvm.org/docs/LibASTMatchersTutorial.html):

```
mkdir ~/clang-llvm
cd ~/clang-llvm
git clone https://github.com/llvm/llvm-project.git
mkdir build && cd build
cmake -G Ninja ../llvm -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_BUILD_TESTS=ON
ninja
ninja check       # Test LLVM only.
ninja clang-test  # Test Clang only.
ninja install
```

To install the Mutation Testing Framework, clone the project into ~/clang-llvm/clang/tools/

```
cd ~/clang-llvm/clang/tools/
git clone http://mod.lge.com/hub/loc.phan/mutation-testing-framework.git
echo 'add_subdirectory(mutation-testing-framework)' >> CMakeLists.txt
cd ~/clang-llvm/build
ninja
```

Install [python-gitlab](https://python-gitlab.readthedocs.io/en/stable/index.html) to run extract-targetlines-from-commit.py 

```
sudo pip install --upgrade python-gitlab
```

## UNRESOLVED POTENTIAL ISSUES

1. Mutate 2 files with same name.
2. SBR deletes a 'big' statement (e.g. if, for, while) with label
