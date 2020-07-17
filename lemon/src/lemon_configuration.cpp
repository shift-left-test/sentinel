#include <iostream>

#include "lemon_configuration.h"
#include "lemon_utility.h"

using namespace std;

Configuration::Configuration()
  : input_filename_withpath(""), input_filename(""), output_directory("/"), 
    debug_on(false), limit_num_of_mutant(0) {
}

Configuration::Configuration(
    std::string input_filename, std::string specified_filename,
    FilenameToLineMap *targetline_map,
    bool debug_opt,  vector<Mutators> mutators, int limit, std::string directory)
  : input_filename_withpath(input_filename), output_directory(directory),
    input_filename_specified(specified_filename), debug_on(debug_opt), 
    selected_mutators(mutators), limit_num_of_mutant(limit) {
  // Extract input filename only (without leading path)
  this->input_filename = getFilenameWithoutLeadingPath(input_filename_withpath);

  // Get specified target lines (if any).
  if (targetline_map->find(this->input_filename) != targetline_map->end())
    targeted_lines = targetline_map->operator[](this->input_filename);

  // Generate mutant database filename.
  mutant_database_filename = directory + "mutant_db.csv";
  // mutant_database_filename = directory;
  // size_t last_dot = this->input_filename.find_last_of(".");
  // if (last_dot == string::npos) {
  //   // Input file does not have ending .c or .cpp
  //   std::cerr << input_filename_withpath << " may not be a C/C++ file\n";
  //   exit(1);
  // }
  // mutant_database_filename += this->input_filename.substr(0, last_dot);
  // mutant_database_filename += "_mut_db.csv";

  cout << "input_filename_withpath: " << input_filename_withpath << endl;
  cout << "input_filename: " << this->input_filename << endl;
  cout << "mutant_database_filename: " << mutant_database_filename << endl;
}

std::string Configuration::getInputFilename() {
  return input_filename;
}

std::string Configuration::getInputFilenameWithPath() {
  return input_filename_withpath;
}

std::string Configuration::getSpecifiedInputFilename() {
  return input_filename_specified;
}

std::string Configuration::getMutationDbFilename() {
  return mutant_database_filename;
}

std::string Configuration::getOutputDir() {
  return output_directory;
}

std::vector<Mutators> Configuration::getSelectedMutators() {
  return selected_mutators;
}

// std::vector<int> Configuration::getTargetLines(std::string filename) {
//   auto it = file_to_targetlines_map.find(filename);
//   if (it != file_to_targetlines_map.end())
//     return it->second;

//   return std::vector<int>();
// }

bool Configuration::isTargetLine(int line) {
  return targeted_lines.size() == 0 || 
         std::find(targeted_lines.begin(), targeted_lines.end(), line) != \
            targeted_lines.end();
}

bool Configuration::debugIsOn() {
  return debug_on;
}

int Configuration::getLimitNumOfMutants() {
  return limit_num_of_mutant;
}

/*std::pair<bool, vector<int>> Configuration::containTargetLine(int start, int end) {
  bool res1 = false;
  vector<int> res2;

  for (auto e: targeted_lines) 
    if (e >= start && e <= end) {
      res1 = true;
      res2.push_back(e);
    }

  return make_pair(res1, res2);
}*/
