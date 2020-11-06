import os
from subprocess import Popen, PIPE


def test_mutate(sentinel_env):
    cmd = [sentinel_env.bin, "mutate",
        sentinel_env.source_dir,
        "--work-dir",
        sentinel_env.work_dir,
        "--output-dir",
        sentinel_env.output_dir,
        "--mutant",
        sentinel_env.mutant]
    print(cmd)
    sentinel = Popen(cmd, stdout=PIPE, stderr=PIPE)
    outs, errs = sentinel.communicate()
    assert len(errs) == 0, errs
