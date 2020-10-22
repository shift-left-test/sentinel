import os
from subprocess import Popen, PIPE


def test_populate(sentinel_env):
    sentinel = Popen([sentinel_env.bin, 
        sentinel_env.source_dir,
        "--work-dir",
        sentinel_env.work_dir,
        "--output-dir",
        sentinel_env.output_dir,
        "populate",
        "--build-dir",
        sentinel_env.build_dir],
        stdout=PIPE, stderr=PIPE)
    outs, errs = sentinel.communicate()
    assert str(outs).find("Mutant Population Report") != -1

