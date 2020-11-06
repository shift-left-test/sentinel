import os
from subprocess import Popen, PIPE


def test_standalone(sentinel_env):
    cmd = [sentinel_env.bin, "run",
        sentinel_env.source_dir,
        "--work-dir",
        sentinel_env.work_dir,
        "--output-dir",
        sentinel_env.output_dir,
        "--build-dir",
        sentinel_env.build_dir,
        "--build-command",
        "cmake --build .",
        "--test-command",
        "ctest",
        "--test-result-dir",
        sentinel_env.build_dir]
    print(cmd)
    sentinel = Popen(cmd, stdout=PIPE, stderr=PIPE)
    outs, errs = sentinel.communicate()
    print(outs)
    assert str(outs).find("Mutant Population Report") != -1, outs
    assert str(outs).find("Mutation Coverage Report") != -1, outs
