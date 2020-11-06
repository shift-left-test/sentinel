import os
from subprocess import Popen, PIPE


def test_evaluate(sentinel_env):
    cmd = [sentinel_env.bin, "evaluate",
        sentinel_env.source_dir,
        "--work-dir",
        sentinel_env.work_dir,
        "--output-dir",
        sentinel_env.output_dir,
        "--mutant",
        sentinel_env.mutant,
        "--expected",
        sentinel_env.expected_dir,
        "--actual",
        sentinel_env.actual_dir]
    print(cmd)
    sentinel = Popen(cmd, stdout=PIPE, stderr=PIPE)
    outs, errs = sentinel.communicate()
    assert len(errs) == 0, errs
