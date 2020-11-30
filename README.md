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
      --build-failure                   If build_failure occurs
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
                                        Default: .
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
                                        Default: .
      -b[PATH], --build-dir=[PATH]      Directory where compile_commands.json
                                        file exists.
                                        Default: .
      --test-result-dir=[PATH]          Test command output directory
      --build-command=[SH_CMD]          Shell command to build source
      --test-command=[SH_CMD]           Shell command to execute test
      --test-result-extention=[EXTENSION...]
                                        Test command output file extensions.
      -t[EXTENSION...],
      --extension=[EXTENSION...]        Extentions of source file which could be
                                        mutated.
      -e[PATH...], --exclude=[PATH...]  exclude file or path
      -s[SCOPE], --scope=[SCOPE]        Diff scope, one of ['commit', 'all'].
                                        Default: all
      -l[COUNT], --limit=[COUNT]        Maximum generated mutable count.
                                        Default: 10
```


## Licenses

The project source code is available under MIT license. See [LICENSE](LICENSE).
