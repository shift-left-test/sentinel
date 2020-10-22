import os
import pytest
from subprocess import Popen
import shutil


def pytest_addoption(parser):
    parser.addoption("--executable", help="sentinel binary path", default="./build/src/sentinel")


class SentinelEnv:
    def __init__(self, executable, tmppath):
        self.bin = executable
        
        self.source_dir = os.path.join(tmppath, "source")
        shutil.copytree(os.path.join(os.path.dirname(__file__), "../test/input/sample1"), self.source_dir)
        self.build_dir = os.path.join(tmppath, "build")
        self.work_dir = os.path.join(tmppath, "work")
        self.output_dir = os.path.join(tmppath, "output")
        self.mutant = "ROR,{}/sample1.cpp,sumOfEvenPositiveNumber,58,17,58,19,<=".format(os.path.abspath(self.source_dir))
        self.expected_dir = os.path.join(tmppath, "expected")
        os.mkdir(self.expected_dir)
        self.actual_dir = os.path.join(tmppath, "actual")
        os.mkdir(self.actual_dir)
        self.eval_file_name = "mutation_result"
        self.eval_file = os.path.join(self.work_dir, self.eval_file_name)

        proc_git = Popen(["git", "init"], cwd=self.source_dir)
        proc_git.wait()
        proc_git = Popen(["git", "add", "."], cwd=self.source_dir)
        proc_git.wait()

        proc_cmake = Popen(["cmake", "-S", self.source_dir, "-B", self.build_dir])
        proc_cmake.wait()


@pytest.fixture(scope="session")
def sentinel_env(request, tmpdir_factory):
    tmppath = tmpdir_factory.mktemp("work")

    def cleanup():
        shutil.rmtree(tmppath)

    request.addfinalizer(cleanup)
    return SentinelEnv(request.config.getoption("--executable"), tmppath)
