import os
from subprocess import Popen, PIPE


def test_report(sentinel_env):
    cmd = [sentinel_env.bin, "report",
        sentinel_env.source_dir,
        "--work-dir",
        sentinel_env.work_dir,
        "--output-dir",
        sentinel_env.output_dir,
        "--evaluation-file",
        sentinel_env.eval_file]
    print(cmd)
    sentinel = Popen(cmd, stdout=PIPE, stderr=PIPE)
    outs, errs = sentinel.communicate()
    assert len(errs) == 0, errs
