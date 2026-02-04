# sentinel

## About

Sentinel is a mutation testing tool based on LLVM/Clang for C/C++ projects.


### Features

* Apply the mutation testing on C/C++ programs
* Support diverse configuration options
* Support application of mutation testing for commit code to cope with cost problem of mutation testing
* Automatically generate HTML/XML format report files

### Mutation testing

Mutation testing is a software testing technique to evaluate and improve the quality of a test suite by generating faulty versions of target program (mutants) and check if the test suite can detect such faults.

Mutation testing can be beneficial for software developers in several ways.

* it provides developers/reviewers ways to evaluate the test suite's effectiveness with respect to the mutation score
* it provides guidance to improve the test suite or to write new test cases

You may find more information about the mutation testing on https://en.wikipedia.org/wiki/Mutation_testing.


## How to install

To prepare your host workspace, you can simply download a preconfigured dockerfile to start the docker container.

    $ git clone https://github.com/shift-left-test/dockerfiles.git
    $ cd dockerfiles
    $ docker build -f clang-dev/11/Dockerfile -t clang-dev-11 .
    $ docker run --rm -it clang-dev-11

Create the sentinel package from the source code.

    $ git clone https://github.com/shift-left-test/sentinel
    $ cd sentinel
    $ cmake .
    $ make all -j
    $ make package

Then, install the package to your machine.

    $ sudo apt-get install ./sentinel-0.4.8-amd64.deb


## How to use

Sentinel in the text-based user interface mode is much easier to use and provides more userful information about each options. You can simply run the program in the text-based user interface mode by:

    $ sentinel gui

You can also run sentinel in the command line with detailed parameters. You may find the detailed information about the command-line arguments below.

### Command-line arguments

```bash
  sentinel COMMAND {OPTIONS}

    Mutation Test

  OPTIONS:

      commands
        populate                          Identify mutable test targets and
                                          application methods in 'git' and print
                                          a list
        mutate                            Apply the selected 'mutable' to the
                                          source. The original file is backed up
                                          in 'work-dir'
        evaluate                          Compare the test result with mutable
                                          applied and the test result not
                                          applied
        report                            Create a mutation test report based on
                                          the'evaluate' result and source code
        run                               Run mutation test in standalone mode
        gui                               Run sentinel in GUI mode
      arguments
        -h, --help                        Display this help menu.
                                          Use 'sentinal COMMAND --help' to see
                                          help for each command.
```

### populate

```bash
  sentinel populate [SOURCE_ROOT_PATH] {OPTIONS}

    Identify mutable test targets and application methods in 'git' and print a
    list

  OPTIONS:

      SOURCE_ROOT_PATH                  source root directory.
      -v, --verbose                     Verbosity
      -w[PATH], --work-dir=[PATH]       Sentinel temporary working directory.
                                        Default: ./sentinel_tmp
      -o[PATH], --output-dir=[PATH]     Directory for saving output.
                                        Default: .
      -b[PATH], --build-dir=[PATH]      Directory where compile_commands.json
                                        file exists.
                                        Default: .
      -s[SCOPE], --scope=[SCOPE]        Diff scope, one of ['commit', 'all'].
                                        Default: all
      -t[EXTENSION...],
      --extension=[EXTENSION...]        Extentions of source file which could be
                                        mutated.
      -e[PATH...], --exclude=[PATH...]  exclude file or path
      -l[COUNT], --limit=[COUNT]        Maximum generated mutable count.
                                        Default: 10
      --mutants-file-name=[PATH]        Populated result file name which will be
                                        created at output-dir.
                                        Default: mutables.db
      --generator=[gen]                 Select mutant generator type, one of
                                        ['uniform', 'random', 'weighted'].
                                        Default: uniform
      --seed=[SEED]                     Select random seed.
                                        Default: 1942447250
```

### mutate
```bash
  sentinel mutate [SOURCE_ROOT_PATH] {OPTIONS}

    Apply the selected 'mutable' to the source. The original file is backed up
    in 'work-dir'

  OPTIONS:

      SOURCE_ROOT_PATH                  source root directory.
      -v, --verbose                     Verbosity
      -w[PATH], --work-dir=[PATH]       Sentinel temporary working directory.
                                        Default: ./sentinel_tmp
      -o[PATH], --output-dir=[PATH]     Directory for saving output.
                                        Default: .
      -m[MUTANT], --mutant=[MUTANT]     Mutant string
```

### evaluate
```bash
  sentinel evaluate [SOURCE_ROOT_PATH] {OPTIONS}

    Compare the test result with mutable applied and the test result not applied

  OPTIONS:

      SOURCE_ROOT_PATH                  source root directory.
      -v, --verbose                     Verbosity
      -w[PATH], --work-dir=[PATH]       Sentinel temporary working directory.
                                        Default: ./sentinel_tmp
      -o[PATH], --output-dir=[PATH]     Directory for saving output.
                                        Default: .
      -m[mutant], --mutant=[mutant]     Mutant string
      -e[PATH], --expected=[PATH]       Expected result directory
      -a[PATH], --actual=[PATH]         Actual result directory
      --evaluation-file=[FILEPATH]      Evaluated output filename which will be
                                        joined with output-dir.
                                        Default: EvaluationResults
      --test-state=[TEST_STATE]         Select the state of the test to be
                                        evaluated, one of ['success',
                                        'build_failure', 'timeout',
                                        'uncovered'].
                                        Default: success
```

### report
```bash
  sentinel report [SOURCE_ROOT_PATH] {OPTIONS}

    Create a mutation test report based on the'evaluate' result and source code

  OPTIONS:

      SOURCE_ROOT_PATH                  source root directory.
      -v, --verbose                     Verbosity
      -w[PATH], --work-dir=[PATH]       Sentinel temporary working directory.
                                        Default: ./sentinel_tmp
      -o[PATH], --output-dir=[PATH]     Directory for saving output.
                                        If output-dir is not given,
                                        pass generating output file.
      --evaluation-file=[PATH]          Mutation test result file
```

### run
```bash
  sentinel run [SOURCE_ROOT_PATH] {OPTIONS}

    Run mutation test in standalone mode

  OPTIONS:

      SOURCE_ROOT_PATH                  source root directory.
      -v, --verbose                     Verbosity
      -w[PATH], --work-dir=[PATH]       Sentinel temporary working directory.
                                        Default: ./sentinel_tmp
      -o[PATH], --output-dir=[PATH]     Directory for saving output.
      -b[PATH], --build-dir=[PATH]      Directory where compile_commands.json
                                        file exists.
                                        Default: .
      --test-result-dir=[PATH]          Test command output directory
      --build-command=[SH_CMD]          Shell command to build source
      --test-command=[SH_CMD]           Shell command to execute test
      --generator=[gen]                 Mutant generator type, one of
                                        ['uniform', 'random', 'weighted'].
                                        Default: uniform
      --test-result-extension=[EXTENSION...]
                                        Test command output file extensions.
      -t[EXTENSION...],
      --extension=[EXTENSION...]        Extentions of source files to be
                                        mutated.
      -e[PATH...], --exclude=[PATH...]  Exclude file or path
      --coverage=[COV.INFO...]          lcov-format coverage result file
      -s[SCOPE], --scope=[SCOPE]        Diff scope, one of ['commit', 'all'].
                                        Default: all
      -l[COUNT], --limit=[COUNT]        Maximum number of mutants to be
                                        generated
                                        Default: 10
      --timeout=[TIME_SEC]              Time limit (sec) for test-command. If 0,
                                        there is no time limit. If auto, time
                                        limit is automatically set using the
                                        test execution time of the original
                                        code.
                                        Default: auto
      --kill-after=[TIME_SEC]           Send SIGKILL if test-command is still
                                        running after timeout. If 0, SIGKILL is
                                        not sent. This option has no meaning
                                        when timeout is set 0.
                                        Default: 60
      --seed=[SEED]                     Select random seed.
                                        Default: random
```


## Development

### Mutation operators

The following mutation operators are supported by sentinel:

- *AOR* - arithmetic operator replacement
- *BOR* - bitwise operator replacement
- *LCR* - logical connector replacement
- *ROR* - relational operator replacement
- *SDL* - statement deletion
- *SOR* - shift operator replacement
- *UOI* - unary operator insertion

### Supported test runners

The following test runners are supported by sentinel:

- googletest
- qtest
- ctest


## Licenses

The project source code is available under MIT license. See [LICENSE](LICENSE).
