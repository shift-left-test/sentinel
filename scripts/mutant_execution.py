import subprocess, sys, os
import json
import xmltodict

def executeMutants(cwd, project_path, build_command, test_command, test_results, test_time):
  survived_mutants = []

  with os.scandir(os.getcwd()+'/temp/mutants') as entries:
    for entry in entries:
      if '_mut_db.csv' in entry.name:
        survived_mutants += executeMutantInDatabase(
            entry.name, build_command, test_command, project_path, 
            test_results, test_time)
        # databases.append(entry.name)

  os.chdir(cwd)
  print("=======================================")
  return survived_mutants

def executeMutantInDatabase(
    db_name, build_command, test_command, project_path, test_results, test_time):
  cwd = os.getcwd()
  os.chdir(cwd+"/temp/mutants")
  db = open(db_name, 'r')
  mutants = [line.strip().split(',')[:2] for line in db][1:]
  db.close()

  if len(mutants) == 0:
    return []

  # copy target file to temp/mutants folder 
  subprocess.run(['cp', mutants[0][0], '.'])

  survived_mutants = []

  for mutant in mutants:
    # replace target file with mutant
    subprocess.run(['cp', mutant[1], mutant[0]])

    # build and test target project again
    os.chdir(project_path)
    try:
      print("---------------------------------------")
      print("Compiling with", mutant[1])
      subprocess.check_output(build_command, stderr=subprocess.STDOUT, 
                              shell=True)
    except subprocess.CalledProcessError as err:
      print("Compilation fail. Uncompilable mutants.")
      
      build_log = open(cwd+"/temp/mutants/"+mutant[1]+".build.log", 'w')
      build_log.write(str(err.stdout))
      build_log.write('\n')
      build_log.write(str(err.stderr))
      build_log.close()

      os.chdir(cwd+"/temp/mutants")
      continue
    else:
      print("Compilation success!")

    # run tests
    # test_result = "test_detail.json"
    os.chdir(project_path)
    try:
      print("Running tests (Timeout: %.5f seconds) ..." % (test_time*2))
      subprocess.check_output(test_command, shell=True, timeout=test_time*2)
    except subprocess.CalledProcessError as err:
      # if not os.path.isfile(test_result):
      #   print("Test result file not found!")
      #   print("STDOUT:", err.stdout)
      #   print("STDERR:", err.stderr)
      #   os.chdir(cwd+"/temp/mutants")
      #   continue
      pass
    except subprocess.TimeoutExpired as timeout_err:
      print("Timeout after %.5f seconds." % timeout_err.timeout)
      print("Mutant KILLED!")

      test_log = open(cwd+"/temp/mutants/"+mutant[1]+".test.log", 'w')
      test_log.write(str(timeout_err.stdout))
      test_log.write('\n')
      test_log.write(str(timeout_err.stderr))
      test_log.close()

      clearTestResults(test_results)
      os.chdir(cwd+"/temp/mutants")
      continue

    print("Testing Complete! Analyzing results ...")

    # analyze test results
    if mutantSurvived(test_results, project_path):
      survived_mutants.append(mutant[1])
    clearTestResults(test_results)

    os.chdir(cwd+"/temp/mutants")
  
  # copy original file back to original location
  # print(os.getcwd())
  # print(mutants[0][0].split('/')[-1], mutants[0][0])
  subprocess.run(['cp', mutants[0][0].split('/')[-1], mutants[0][0]])
  os.chdir("../../")
  return survived_mutants

def mutantSurvived(test_results, project_path):
  test_format = test_results[0]
  for test_result in test_results[1]:
    # check result file exists for both original target program and mutant
    orig_result = '/'.join([project_path, '../orig-results', 
                            test_result.split('/')[-1]])

    # One generate test output. Other does not --> mutant killed
    if bool(os.path.isfile(test_result)) != bool(os.path.isfile(orig_result)):
      print("result file mismatch:", test_result)
      print("Mutant KILLED!")
      return False

    # Both does not generate this test output file --> skip
    if not os.path.isfile(test_result): continue

    # Compare output file based on format
    if test_format == 'json':
      if sameJSONResults(test_result, orig_result):
        continue
      else:
        print("Mutant KILLED!")
        return False
    else: # xml format
      if sameXMLResults(test_result, orig_result):
        continue
      else:
        print("Mutant KILLED!") 
        return False

  print("Mutant SURVIVED")
  return True   

def sameJSONResults(new_result, old_result):
  mutant_fail_tests = getListOfFailTestsFromJSON(new_result)
  orig_fail_tests = getListOfFailTestsFromJSON(old_result)

  if mutant_fail_tests != orig_fail_tests:
    print("Mutant fails on tests:", mutant_fail_tests)
    print("Target fails on tests:", orig_fail_tests)

  return mutant_fail_tests == orig_fail_tests

def getListOfFailTestsFromJSON(result_filename):
  ret = []
  with open(result_filename, 'r') as result_file:
    result = json.load(result_file)

  if result['failures'] == 0:
    return ret

  for testsuite in result['testsuites']:
    if testsuite['failures'] == 0:
      continue

    for test in testsuite['testsuite']:
      if 'failures' in test:
        ret.append(testsuite['name']+'.'+test['name'])

  ret.sort()
  return ret

def sameXMLResults(new_result, old_result):
  mutant_fail_tests = getListOfFailTestsFromXML(new_result)
  orig_fail_tests = getListOfFailTestsFromXML(old_result)

  if mutant_fail_tests != orig_fail_tests:
    print("Mutant fails on tests:", mutant_fail_tests)
    print("Target fails on tests:", orig_fail_tests)

  return mutant_fail_tests == orig_fail_tests

def getListOfFailTestsFromXML(result_filename):
  ret = []
  # print("checking", result_filename)
  with open(result_filename, 'r') as result_file:
    result = xmltodict.parse(result_file.read())['testsuites']

  if result['@failures'] == 0:
    return ret

  for testsuite in result['testsuite']:
    if testsuite['@failures'] == 0:
      continue

    for test in testsuite['testcase']:
      if 'failure' in test:
        ret.append(testsuite['@name']+'.'+test['@name'])

  ret.sort()
  return ret

def clearTestResults(test_results):
  for result in test_results[1]:
    subprocess.run(['rm', result], stdout=subprocess.DEVNULL, 
                   stderr=subprocess.DEVNULL)
