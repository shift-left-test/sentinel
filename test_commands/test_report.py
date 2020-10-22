import os
from subprocess import Popen, PIPE


def test_report(sentinel_env):
    sentinel = Popen([sentinel_env.bin,
        sentinel_env.source_dir,
        "--work-dir",
        sentinel_env.work_dir,
        "--output-dir",
        sentinel_env.output_dir,
        "report",
        "--evaluation-file",
        sentinel_env.eval_file],
        stdout=PIPE, stderr=PIPE)
    outs, errs = sentinel.communicate()
    assert len(errs) == 0, errs
