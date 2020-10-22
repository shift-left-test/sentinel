import os
from subprocess import Popen, PIPE


def test_integration(sentinel_env):
    populate = Popen([sentinel_env.bin, 
        sentinel_env.source_dir,
        "--work-dir",
        sentinel_env.work_dir,
        "--output-dir",
        sentinel_env.output_dir,
        "populate",
        "--build-dir",
        sentinel_env.build_dir],
        stdout=PIPE, stderr=PIPE)
    outs, errs = populate.communicate()
    assert str(outs).find("Mutant Population Report") != -1
    populate_output = os.path.join(sentinel_env.output_dir, "mutables.db")
    assert os.path.exists(populate_output)

    with open(populate_output) as f:
        mutants = f.readlines()
        for mutant in mutants:
            print("mutant: {}".format(mutant))
            mutate = Popen([sentinel_env.bin,
                sentinel_env.source_dir,
                "--work-dir",
                sentinel_env.work_dir,
                "--output-dir",
                sentinel_env.output_dir,
                "mutate",
                "--mutant",
                sentinel_env.mutant],
                stdout=PIPE, stderr=PIPE)
            outs, errs = mutate.communicate()

            cwd = os.getcwd()
            os.chdir(sentinel_env.build_dir)
            build = Popen(["cmake", "--build", "."],
                stdout=PIPE, stderr=PIPE)
            build.communicate()
            test = Popen(["ctest"],
                stdout=PIPE, stderr=PIPE)
            test.communicate()
            os.chdir(cwd)

            evaluate = Popen([sentinel_env.bin,
                sentinel_env.source_dir,
                "--work-dir",
                sentinel_env.work_dir,
                "--output-dir",
                sentinel_env.work_dir,
                "evaluate",
                "--mutant",
                sentinel_env.mutant,
                "--expected",
                sentinel_env.expected_dir,
                "--actual",
                sentinel_env.actual_dir,
                "--evaluation-file",
                sentinel_env.eval_file_name],
                stdout=PIPE, stderr=PIPE)
            outs, errs = evaluate.communicate()

    assert os.path.exists(sentinel_env.eval_file)
    report = Popen([sentinel_env.bin,
        sentinel_env.source_dir,
        "--work-dir",
        sentinel_env.work_dir,
        "--output-dir",
        sentinel_env.output_dir,
        "report",
        "--evaluation-file",
        sentinel_env.eval_file],
        stdout=PIPE, stderr=PIPE)
    outs, errs = report.communicate()
    xml_report = os.path.join(sentinel_env.output_dir, "mutations.xml")
    assert os.path.exists(xml_report)
    with open(xml_report) as f:
        reportlines = f.readlines()
        print(reportlines)
