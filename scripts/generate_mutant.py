import argparse
import json
import csv
import random
from copy import deepcopy

mutant_id = 1

mutation_operator_range = {
  "AOR": ["+", "-", "*", "/", "%"],
  "BOR": ["|", "&", "^"],
  "SOR": [">>", "<<"],
  "ROR": [">", ">=", "<", "<=", "==", "!="],
  "LCR": ["||", "&&"],
  "SBR": [""],
  "UOR": {
    "++": "--",
    "--": "++",
    "!": ""
  }
}

def get_mutated_token(operator, token):
  assert(operator in mutation_operator_range)
  if operator == "UOR":
    return mutation_operator_range["UOR"][token]

  # return a randomly selected token to mutate target token
  token_range = deepcopy(mutation_operator_range[operator])
  # print(operator, token, token_range)
  assert(token in token_range)
  token_range.remove(token)
  random.shuffle(token_range)
  return token_range[0]

def generate_mutant(mutant):
  global mutant_id
  # Randomly select a token to mutate to from a token pool based on mutation operator
  mutated_token = get_mutated_token(mutant["operator"], mutant["targeted_token"])
  filename = mutant["filename"]

  # Copy original code to mutated code line by line until reaching targeted line.
  # Apply mutation to the targeted line, and continue copy the rest of the file.
  mutant_filename = filename.split('/')[-1]
  last_dot_pos = mutant_filename.rfind('.')
  file_ending = mutant_filename[last_dot_pos+1:]
  mutant_filename = args.o + '/' + mutant_filename[:last_dot_pos]+".MUT"+str(mutant_id)+'.'+file_ending
  mutant_id += 1

  print("%s: mutating %s to %s on line %d", mutant_filename, 
                                            mutant["targeted_token"], 
                                            mutated_token, mutant["start_line"])

  with open(filename, 'r') as orig_file, \
       open(mutant_filename, 'w') as mutant_file:
    line_idx = 1
    line = orig_file.readline()

    # If code line is out of target range, just write to mutant file.
    # If code line is in target range (start_line < code_line < end_line), skip.
    # If code line is on start_line, write the code appearing before start_col,
    # and write mutated token.
    # If code line is on end_line, write the code appearing after end_col.
    while line: 
      if line_idx < mutant["start_line"] or line_idx > mutant["end_line"]:
        mutant_file.write(line)

      if line_idx == mutant["start_line"]:
        mutant_file.write(line[:(mutant["start_col"]-1)])
        mutant_file.write(mutated_token)

      if line_idx == mutant["end_line"]:
        mutant_file.write(line[(mutant["end_col"]-1):])

      line = orig_file.readline()
      line_idx += 1

def main():
  with open(args.csv) as csv_file:
    # load CSV data
    data = csv.reader(csv_file, delimiter=',')

    # Group mutant information according to line number.
    # Randomly select one on each line to mutate.
    sorted_data = {}
    for row in data:
      mutant = {
        "operator": row[0].strip(),
        "targeted_token": row[1].strip(),
        "filename": row[2].strip(),
        "start_line": int(row[3].strip()),
        "start_col": int(row[4].strip())
      }

      # ====================================
      # TEMPORARY: get end loc of targeted token
      mutant["end_line"] = mutant["start_line"]
      mutant["end_col"] = mutant["start_col"] + len(mutant["targeted_token"])
      # ====================================

      if mutant["start_line"] not in sorted_data:
        sorted_data[mutant["start_line"]] = [mutant]
      else:
        sorted_data[mutant["start_line"]].append(mutant)

    # print(json.dumps(sorted_data, indent=2))

    for _, mutants in sorted_data.items():
      random_idx = random.randint(0, len(mutants)-1)
      generate_mutant(mutants[random_idx])

if __name__ == '__main__':
  parser = argparse.ArgumentParser(
      description='Generate mutants using CSV input.')
  parser.add_argument('csv', action='store', 
                      help='CSV file containing mutant information')
  parser.add_argument('-o', action='store', default=".",
                      help='Specify location to store mutated source files.')
  args = parser.parse_args()
  
  main()