import os
from subprocess import Popen, PIPE


def test_populate(sentinel_env):
    cmd = [sentinel_env.bin, "populate",
        sentinel_env.source_dir,
        "--work-dir",
        sentinel_env.work_dir,
        "--output-dir",
        sentinel_env.output_dir,
        "--build-dir",
        sentinel_env.build_dir]
    print(cmd)
    sentinel = Popen(cmd, stdout=PIPE, stderr=PIPE)
    outs, errs = sentinel.communicate()
    assert str(outs).find("Mutant Population Report") != -1

