import gitlab, sys

def extractTargetLines(commit):
  out_file = open("temp/target_lines.txt", "w+")
  target_files = []

  for file in commit.diff():
    if file['new_path'][-1] != 'c' and file['new_path'][-3:] != 'cpp':
      return

    if file['deleted_file']:
      return

    target_files.append(file['new_path'])

    # If this file is new then all lines are targeted. 
    if file['new_file']:
      out_file.write(
          file['new_path'] + ':' + \
          ','.join(map(str, list(range(1, len(file['diff'].split('\n'))-1)))) + \
          '\n')
      return

    # Remaining are changed files. Target lines that start with '+'    
    out_file.write(file['new_path'] + ':')
    collectAddedLines(file['diff'], out_file)    

  out_file.close()
  return target_files 

def collectAddedLines(diff, out_file):
  iter_line = 0
  target_lines = []

  for line in diff.split('\n'):
    if len(line) == 0:
      iter_line += 1
      continue 

    # Line starting with @@ specifies new range of changed lines.
    if line[:2] == "@@":
      iter_line = int(line.split('+')[1].split(',')[0])-1
      continue

    # Ignore all lines starting with '-'
    # Record those starting with '+'
    if line[0] != '-':
      iter_line += 1  
      if line[0] == '+':
        target_lines.append(str(iter_line))

  out_file.write(','.join(target_lines) + '\n')

