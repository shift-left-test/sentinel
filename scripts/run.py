import subprocess, sys, os
import argparse
import gitlab
import time
from extract_targetlines_from_commit import extractTargetLines
from mutant_execution import executeMutants

def main():
  gl = accessGitlab()
  if gl == None: return
  project = gl.projects.get(args.projectid)  
  cwd = os.getcwd()
  # print(args.compiledb)
  project_path = '/'.join([cwd, 'temp', project.path, ''])
  args.compiledb = project_path + args.compiledb
  for i in range(len(args.testresults)):
    args.testresults[i] = project_path + args.testresults[i]
  
  createTempFolder()

  # Create temp folder, and clone target project
  if not 'clone' in args.skip:
    cloneProject(project.http_url_to_repo, args.commitid, project_path)

  if not 'build' in args.skip:
    buildProject(project_path, cwd, args.buildcommand)
  else:
    buildProject(project_path, cwd, args.rebuildcommand)
  
  if not 'test' in args.skip:
    test_time = runProjectTest(project_path, cwd)

  # Mutant Generation
  target_files = extractTargetLines(project.commits.get(args.commitid))
  if not 'mutate' in args.skip:
    generateMutants(project_path, target_files)

  # Mutant Execution
  survived_mutants = executeMutants(
      cwd, project_path, args.rebuildcommand, args.testcommand,                
      (args.testresultformat, args.testresults), test_time)

  reportSummary(cwd, survived_mutants)

  clearProject(cwd)

def createTempFolder():
  if os.path.isdir('temp'):
    use_ans = ""
    while use_ans != 'y' and use_ans != 'n':
      use_ans = input("Folder named temp already exists here! Use it? (y or n) ")
      # use_ans = input("LEMON wants to use the folder. Is it ok to delete? (y or n) ")

    if use_ans == 'n':
      delete_ans = ""
      while delete_ans != 'y' and delete_ans != 'n':
        delete_ans = input("Delete it? (y or n) ")
      
      if delete_ans == 'n':
        print("Change location or backup the temp file before starting again.")
        exit(1)
      else:
        # delete folder and make new one
        subprocess.run(['rm', '-rf', 'temp'])
        subprocess.run(['mkdir', 'temp'])  # Create temporary directory
    else:
      print("Use existing temp folder")
    print("=======================================")
  else:
    subprocess.run(['mkdir', 'temp'])  # Create temporary directory

def cloneProject(url, commitid, project_path):  
  print("Cloning:", url)
  os.chdir("temp")
  subprocess.run(['git', 'clone', url])
  os.chdir(project_path)
  subprocess.run(['git', 'checkout', commitid])

  os.chdir("../../")
  print("=======================================")

def buildProject(project_path, cwd, build_command):
  print("Building ...")
  os.chdir(project_path)

  # ========== BUILD SCRIPT EXECUTION ============
  start = time.time()
  try:
    subprocess.check_output(build_command, stderr=subprocess.STDOUT, 
                            shell=True)
  except subprocess.CalledProcessError as err:
    print("Cannot build using this build command:", build_command)
    print("STDOUT:", err.stdout)
    print("STDERR:", err.stderr)
    clearProject(cwd)
    exit(1)
  end = time.time()
  print("Build Time: %.5f seconds." % (end-start))
  # ==================================================

  # check if compilation database is generated
  if not os.path.isfile(args.compiledb):
    print("Compilation database not found.")
    exit(1)
  
  os.chdir(cwd)
  print("=======================================")

def runProjectTest(project_path, cwd):
  print("Testing ...")
  os.chdir(project_path)

  start = time.time()
  # ========== TEST SCRIPT EXECUTION ============
  try:
    subprocess.check_output(args.testcommand, stderr=subprocess.STDOUT, 
                            shell=True)
  except subprocess.CalledProcessError as err:
    print("Warning: there may be a failing test or error running test script.")
    pass
  # ==================================================
  end = time.time()
  print("Original Test Time: %.5f seconds." % (end-start))

  # copy original test results to a temporary folder in temp
  os.chdir(cwd)
  os.chdir('temp')
  subprocess.run(['mkdir', 'orig-results'])
  for resultfile in args.testresults:
    try:
      subprocess.check_output(['mv', resultfile, '-t', 'orig-results'])
    except subprocess.CalledProcessError as err:
      print("Warning: Error moving a result file to destination")
      print("STDOUT:", err.stdout)
      print("STDERR:", err.stderr)
      # exit(1)

  os.chdir(cwd)
  print("=======================================")

  return end - start

def clearProject(cwd):
  if args.cleanup:
    os.chdir(cwd)
    subprocess.run(['rm', '-rf', 'temp'])

def reportSummary(cwd, survived_mutants):
  print("=============== SUMMARY ===============")
  num_mutants = 0
  with os.scandir(cwd+'/temp/mutants') as entries:
    for entry in entries:
      if '.MUT' in entry.name and entry.name[-4:] != '.log':
        num_mutants += 1

  print("Number of GENERATED mutants = %d" % num_mutants)
  print("Number of KILLED    mutants = %d" % (num_mutants-len(survived_mutants)))
  print("Number of SURVIVED  mutants = %d" % len(survived_mutants))

  if len(survived_mutants) > 0:
    print("The following mutants survived:")
    print('\n'.join(survived_mutants))

  print("=======================================")

def accessGitlab():
  if args.terminal == 'modlge':
    # Token expires on 2020.04.30
    return gitlab.Gitlab('http://mod.lge.com/hub/', oauth_token='Ls9yLyFZuiGAvygsu1v-')
  elif args.terminal == 'gitlab':
    return gitlab.Gitlab('https://gitlab.com/', oauth_token='_nbPVL7s_dVVQayR8eoM')
  else:
    print("Error: target project should be from Gitlab or mod.lge.com/hub/")
    return None

def generateMutants(project_path, target_files):
  # print(project_path)
  os.chdir(project_path + '/..')
  subprocess.run(['mkdir', 'mutants'])

  # target files
  command = ['lemon'] + list(map(lambda x: (project_path + x), target_files))
  # target lines
  command += ['-target', project_path+'../'+'target_lines.txt']
  # output directory
  command += ['-o', project_path+'../'+'mutants']
  
  # mutator list
  if len(args.mutator) > 0:
    for e in args.mutator:
      command += ['--'+e]
      
  # compilation db
  command += ['-p', args.compiledb]

  print("Generating mutants using the following command:")
  print(' '.join(command))
  print("=======================================")

  subprocess.run(command)
  os.chdir('..')

if __name__ == '__main__':
  parser = argparse.ArgumentParser(
      description='Conduct mutation analysis on changed lines in a project commit.')
  parser.add_argument('terminal', action='store', choices=['gitlab', 'modlge'],
                      help='is project on gitlab.com or mod.lge.com')
  parser.add_argument('projectid', action='store', type=int,
                      help='ID of Gitlab project.')
  parser.add_argument('commitid', action='store', help='ID of commit')
  parser.add_argument('buildcommand', action='store')
  parser.add_argument('rebuildcommand', action='store')
  parser.add_argument('testcommand', action='store')
  parser.add_argument('testresults', action='store')
  parser.add_argument('testresultformat', action='store', 
                      choices=['xml', 'json'])
  parser.add_argument('-compiledb', action='store', 
                      default='compile_commands.json',
                      help='specify location of compilation database with respect to project directory.')

  parser.add_argument('-c', '--cleanup', action='store_true',
                      help='set to clean temp directory after execution.')
  parser.add_argument('-skip', action='append', default=[],
                      choices=['clone', 'build', 'test', 'mutate'])
  parser.add_argument('-mutator', action='append', default=[],
                      choices=['aor', 'bor', 'lcr', 'ror', 'sbr', 'sor', 'uoi'])
  args = parser.parse_args()
  args.testresults = args.testresults.split()

  if len(args.skip) > 0:
    print("Skipping the following step:", args.skip)
  
  main()
