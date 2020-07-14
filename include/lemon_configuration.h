#ifndef LEMON_CONFIGURATION_H_
#define LEMON_CONFIGURATION_H_

#include <string>
#include <map>
#include <vector>
#include <tuple>

#include "clang/Basic/SourceLocation.h"
enum Mutators {
  aor, bor, lcr, ror, sbr, sor, uoi
}; 

typedef std::map<std::string, std::vector<int>> FilenameToLineMap;

class Configuration
{
public:
  Configuration();
  
  Configuration(
      std::string input_filename, FilenameToLineMap *targetline_map,
      bool debug_opt, std::vector<Mutators> mutators, int limit, 
      std::string directory = "./");

  // Getters
  std::string getInputFilename();
  std::string getMutationDbFilename();
  std::string getOutputDir();
  std::string getInputFilenameWithPath();
  std::vector<Mutators> getSelectedMutators();
  // std::vector<int> getTargetLines(std::string filename);
  bool isTargetLine(int line);
  bool debugIsOn();
  // std::pair<bool, std::vector<int>> containTargetLine(int start, int end);
  int getLimitNumOfMutants();

private:
  std::string input_filename_withpath;
  std::string input_filename;
  std::string mutant_database_filename;
  std::string output_directory;
  bool debug_on;
  std::vector<Mutators> selected_mutators;
  int limit_num_of_mutant;

  // A map from each target file name to a list of changed lines (if available)
  // FilenameToLineMap file_to_targetlines_map;

  std::vector<int> targeted_lines;
};

#endif  // LEMON_CONFIGURATION_H_