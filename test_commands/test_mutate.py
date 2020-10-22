import os
from subprocess import Popen, PIPE


def test_mutate(sentinel_env):

    sentinel = Popen([sentinel_env.bin,
        sentinel_env.source_dir,
        "--work-dir",
        sentinel_env.work_dir,
        "--output-dir",
        sentinel_env.output_dir,
        "mutate",
        "--mutant",
        sentinel_env.mutant],
        stdout=PIPE, stderr=PIPE)
    outs, errs = sentinel.communicate()
    assert len(errs) == 0, errs
