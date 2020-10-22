# sentinel

[![Build Status](http://10.178.85.91:8080/buildStatus/icon?job=sentinel)](http://10.178.85.91:8080/job/sentinel/)

> The mutation testing tool for the meta-shift project


## How to run tests

    
```bash
$ docker run --rm -it cart.lge.com/swte/yocto-dev:18.04
$ git clone http://mod.lge.com/hub/yocto/addons/sentinel.git
$ cd sentinel
$ ./build.sh
```


## How to use sentinel
```bash
Usage: build/src/sentinel [OPTIONS] source-root [SUBCOMMAND]

Positionals:
  source-root TEXT REQUIRED   source root directory.

Options:
  -h,--help                   Print this help message and exit
  --help-all                  Expand all help
  -v,--verbose                Verbosity
  -w,--work-dir TEXT=./sentinel_tmp
                              Sentinel temporary working directory.
  -o,--output-dir TEXT=.      Directory for saving output.

Subcommands:
populate
  Identify mutable test targets and application methods in'git' and print a list
  Options:
    -b,--build-dir TEXT=.       Directory where compile_commands.json file exists.
    -s,--scope TEXT=all         Diff scope, one of ['commit', 'all'].
    -t,--extension TEXT=[cxx,hxx,cpp,hpp,cc,hh,c,h,c++,h++,cu,cuh] ...
                                Extentions of source file which could be mutated.
    -e,--exclude TEXT ...       exclude file or path
    -l,--limit INT=10           Maximum generated mutable count.
    --mutants-file-name TEXT=mutables.db
                                Populated result file name which will be created at output-dir.

mutate
  Apply the selected 'mutable' to the source. The original file is backed up in 'work-dir'
  Options:
    -m,--mutant TEXT REQUIRED   Mutant string

evaluate
  Compare the test result with mutable applied and the test result not applied
  Options:
    -m,--mutant TEXT REQUIRED   Mutant string
    -e,--expected TEXT REQUIRED Expected result directory
    -a,--actual TEXT REQUIRED   Actual result directory
    --evaluation-file TEXT=EvaluationResults
                                Evaluated output filename(joined with output-dir)

report
  Create a mutation test report based on the'evaluate' result and source code
  Options:
    --evaluation-file TEXT REQUIRED
                                Mutation test result file

run
  Run mutation test in standalone mode
  Options:
    -b,--build-dir TEXT=.       Directory where compile_commands.json file exists.
    -s,--scope TEXT=all         diff scope, one of ['commit', 'all'].
    -t,--extension TEXT=[cxx,hxx,cpp,hpp,cc,hh,c,h,c++,h++,cu,cuh] ...
                                Extentions of source file which could be mutated.
    -e,--exclude TEXT ...       exclude file or path
    -l,--limit INT=10           Maximum generated mutable count.
    --build-command TEXT REQUIRED
                                Shell command to build source
    --test-command TEXT REQUIRED
                                Shell command to execute test
    --test-result-dir TEXT REQUIRED
                                Test command output directory
    --test-result-extention TEXT=[xml,XML] ...
                                Test command output file extensions
```


## Licenses

The project source code is available under MIT license. See [LICENSE](LICENSE).
