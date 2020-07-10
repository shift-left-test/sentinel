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
  filename = mutant["filename"]
  mutant_filename = args.o + mutant["mutant_filename"]
  print("%s: mutating \'%s\' to \'%s\' on line %d" % (mutant_filename, 
                                              mutant["targeted_token"], 
                                              mutant["mutated_token"], 
                                              mutant["start_line"]))

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
        mutant_file.write(mutant["mutated_token"])

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
    for row in data:
      mutant = {
        "filename": row[0].strip(),
        "mutant_filename": row[1].strip(),
        "operator": row[2].strip(),
        "start_line": int(row[3].strip()),
        "start_col": int(row[4].strip()),
        "end_line": int(row[5].strip()),
        "end_col": int(row[6].strip()),
        "targeted_token": row[7].strip(),
        "mutated_token": row[8].strip(),
      }

      generate_mutant(mutant)
      # print(json.dumps(mutant, indent=2))

      # if mutant["start_line"] not in sorted_data:
      #   sorted_data[mutant["start_line"]] = [mutant]
      # else:
      #   sorted_data[mutant["start_line"]].append(mutant)

    # for _, mutants in sorted_data.items():
    #   random_idx = random.randint(0, len(mutants)-1)
    #   generate_mutant(mutants[random_idx])

if __name__ == '__main__':
  parser = argparse.ArgumentParser(
      description='Generate mutants using CSV input.')
  parser.add_argument('csv', action='store', 
                      help='CSV file containing mutant information')
  parser.add_argument('-o', action='store', default=".",
                      help='Specify location to store mutated source files.')
  args = parser.parse_args()
  
  main()
