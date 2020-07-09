#include <time.h>
#include <algorithm>
#include <set>
#include <fstream>
#include <stdlib.h>
#include <time.h>

#include "mutant_database.h"
#include "lemon_utility.h"

using namespace clang;
using namespace std;

// void GenerateRandomNumbers(set<int> &s, int desired_size, int cap) {
//   while (s.size() != desired_size)
//     s.insert(rand() % cap);
// }

void formatToken(string &token) {
  // === output formatting ===
  // If token contains comma or newline character, database will not look good.
  // Cut the token at the first comma or newline character found.
  // Can disable later if requested.
  // cout << "token is " << token << endl;
  size_t pos_comma = token.find(",");
  size_t pos_newline = token.find("\n");
  size_t len = 0;

  if (pos_comma == string::npos && pos_newline != string::npos)
    len = pos_newline;
  else if (pos_comma != string::npos && pos_newline == string::npos)
    len = pos_comma;
  else if (pos_comma != string::npos && pos_newline != string::npos)
    len = min(pos_newline, pos_comma);
  
  if (len != 0) {
    token = token.substr(0, len);
    if (token[0] == '\"')
      token.append("\"");
  }
  // cout << "token now is " << token << endl;
  // ======
}

MutantDatabase::MutantDatabase(
    clang::CompilerInstance& comp_inst, Configuration* configuration)
  : CI(comp_inst), src_mgr(comp_inst.getSourceManager()),    
    lang_opts(comp_inst.getLangOpts()), config(configuration), mutant_id(1) {
  // set database filename with output directory prepended
  /*database_filename_ = output_dir;
  size_t last_dot_pos = input_filename_.find_last_of(".");
  database_filename_ += input_filename_.substr(0, last_dot_pos);
  database_filename_ += "_mut_db.csv";*/

  rewriter_.setSourceMgr(src_mgr, lang_opts);
}

void MutantDatabase::addMutantEntry(
    std::string operator_name, clang::SourceLocation start_loc,
    clang::SourceLocation end_loc, std::string targeted_token,
    std::string mutated_token, int target_line) {
  // int line_num = getLineNumber(src_mgr, start_loc);

  /*for (int i = 0; i < extra_start_locs.size(); ++i)
  {
    if (!ValidateSourceRange(extra_start_locs[i], extra_end_locs[i]))
      return;
  }*/

  // This mutant is not in the targeted file.
  if (src_mgr.getMainFileID().getHashValue() != src_mgr.getFileID(start_loc).getHashValue() ||
      src_mgr.getMainFileID().getHashValue() != src_mgr.getFileID(end_loc).getHashValue())
    return;

  if (config->debugIsOn()) {
    cout << "Adding a " << operator_name << " mutant to database." << endl;
    cout << "start location: " << start_loc.printToString(src_mgr) << endl; 
    cout << "end location: " << end_loc.printToString(src_mgr) << endl;
    cout << "token: " << targeted_token << endl;
    cout << "mutated token: " << mutated_token << endl;
    cout << "=========================================\n";
  }

  MutantEntry new_entry(
      operator_name, targeted_token, mutated_token, start_loc, end_loc);    

  /*for (int i = 0; i < extra_start_locs.size(); i++)
    if (extra_start_locs[i].isInvalid() || extra_end_locs[i].isInvalid())
    {
      cout << "MutantDatabase::AddMutantEntry Warning: fail to add mutant ";
      cout << "due to invalid SourceLocation\n";
      cout << extra_start_locs[i].printToString(src_mgr_) << endl;
      cout << extra_end_locs[i].printToString(src_mgr_) << endl;
      cout << name << endl;
      cout << new_entry << endl;
      return;
    }*/

  auto line_map_iter = mutant_entry_table.find(target_line);

  // if this is the first mutant on this line to be recorded,
  // add new entries all the way down 3 levels.
  if (line_map_iter == mutant_entry_table.end()) {
    mutant_entry_table[target_line] = MutantEntryList();
    mutant_entry_table[target_line].push_back(new_entry);
  }
  // if this is not the first mutant on this line, 
  // check if there are any same mutant existed before adding.
  else {
    for (auto entry: mutant_entry_table[target_line]) {
      if (new_entry == entry) {
        return;
      }
    }

    mutant_entry_table[target_line].push_back(new_entry);
  }
}

/*void MutantDatabase::writeMutantToDatabaseFile(const MutantEntry &entry) {
  // Open mutation database file in APPEND mode
  ofstream mutant_db_file(config->getMutationDbFilename().data(), ios::app);

  // write orignal target filename
  mutant_db_file << config->getInputFilenameWithPath() << ","; 

  // write mutant file name
  mutant_db_file << getNextMutantFilename() << ","; 

  // write name of operator  
  mutant_db_file << entry.getOperatorName() << ",";

  // write information about mutation location
  mutant_db_file << getLineNumber(src_mgr, entry.getStartLocation()) << ",";
  mutant_db_file << getColumnNumber(src_mgr, entry.getStartLocation()) << ",";

  string targeted_token = entry.getTargetedToken();
  formatToken(targeted_token);
  mutant_db_file << targeted_token << ",";

  string mutated_token = entry.getMutatedToken();
  formatToken(mutated_token);
  mutant_db_file << mutated_token << endl;

  // close database file
  mutant_db_file.close(); 
}*/

void MutantDatabase::writeMutantToDatabaseFile(const MutantEntry &entry) {
  // Open mutation database file in APPEND mode
  ofstream mutant_db_file(config->getMutationDbFilename().data(), ios::app); 

  // write name of operator  
  mutant_db_file << entry.getOperatorName() << ",";

  // write information about mutation location
  mutant_db_file << getLineNumber(src_mgr, entry.getStartLocation()) << ",";
  mutant_db_file << getColumnNumber(src_mgr, entry.getStartLocation()) << ",";

  string targeted_token = entry.getTargetedToken();
  formatToken(targeted_token);
  mutant_db_file << targeted_token << ",";

  string mutated_token = entry.getMutatedToken();
  formatToken(mutated_token);
  mutant_db_file << mutated_token << endl;

  // close database file
  mutant_db_file.close(); 
}

void MutantDatabase::generateMutantSrcFile(const MutantEntry &entry) {
  Rewriter rewriter;
  rewriter.setSourceMgr(src_mgr, lang_opts);

  SourceLocation start_loc = src_mgr.getExpansionLoc(entry.getStartLocation());
  SourceLocation end_loc = src_mgr.getExpansionLoc(entry.getEndLocation());
  string mutated_token = entry.getMutatedToken();

  if (config->debugIsOn()) {
    cout << "start location: " << start_loc.printToString(src_mgr) << endl; 
    cout << "end location: " << end_loc.printToString(src_mgr) << endl;
    cout << "mutated token: " << mutated_token << endl;
  }

  int length = src_mgr.getFileOffset(end_loc) - \
               src_mgr.getFileOffset(start_loc);
  
  rewriter.ReplaceText(start_loc, length, mutated_token);

  string mutant_filename{config->getOutputDir()};
  mutant_filename += getNextMutantFilename();

  // Make and write mutated code to output file.
  const RewriteBuffer *RewriteBuf = rewriter.getRewriteBufferFor(
      src_mgr.getMainFileID());

  ofstream output(mutant_filename.data());
  output << string(RewriteBuf->begin(), RewriteBuf->end());
  output.close(); 
}

/*void MutantDatabase::WriteAllEntriesToMutantFile()
{
  for (auto line_map_iter: mutant_entry_table)
    for (auto column_map_iter: line_map_iter.second)
      for (auto mutantname_map_iter: column_map_iter.second)
        for (auto entry: mutantname_map_iter.second)
        {
          WriteEntryToMutantFile(entry);
          IncrementNextMutantfileId();
        }
}*/

// bool CompareEntry(long i, long j) {
//   return i < j;
// }

// generate mutant file and write to database file
void MutantDatabase::generateToolOutput() {
  int mutant_count = 0;

  // cout << "there are " << mutant_entry_table.size() << "target lines\n";

  for (auto line_map_iter: mutant_entry_table) {
    // cout << line_map_iter.second.size() << endl;
    // cout << "making mutant " << mutant_id << " on line " << line_map_iter.first << endl;

    // For each target line, randomly generate 1 mutant.
    srand(time(NULL));  
    int random_mutant_idx = rand() % line_map_iter.second.size();
    writeMutantToDatabaseFile(line_map_iter.second[random_mutant_idx]);
    generateMutantSrcFile(line_map_iter.second[random_mutant_idx]);
    incrementNextMutantfileId();
    mutant_count += 1;
  }

  cout << "Number of mutants generated for " << config->getInputFilename();
  cout << ": " << mutant_count << endl;
}

const MutantEntryTable& MutantDatabase::getEntryTable() const {
  return mutant_entry_table;
}

clang::CompilerInstance& MutantDatabase::getCompilerInstance() {
  return CI;
}

Configuration* MutantDatabase::getConfiguration() {
  return config;
}

string MutantDatabase::getNextMutantFilename() {
  // if input filename is "test.c" and next_mutantfile_id_ is 1,
  // then the next mutant filename is "test.MUT1.c"
  // this function will, however, return "test.MUT1" 
  // for use in both database record and mutant file generation
  string mutant_filename;
  size_t last_dot_pos = config->getInputFilename().find_last_of(".");

  mutant_filename = config->getInputFilename().substr(0, last_dot_pos);
  mutant_filename += ".MUT";
  mutant_filename += to_string(mutant_id);
  mutant_filename += config->getInputFilename().substr(last_dot_pos);

  return mutant_filename;
}

void MutantDatabase::incrementNextMutantfileId() {
  mutant_id++;
}

// bool MutantDatabase::validateSourceRange(SourceLocation &start_loc, SourceLocation &end_loc)
// {
//   // I forget the reason why ...
//   // if (start_loc.isMacroID() && end_loc.isMacroID())
//   //   return false;
//   // cout << "checking\n";
//   // PrintLocation(src_mgr_, start_loc);
//   // PrintLocation(src_mgr_, end_loc);

//   if (start_loc.isInvalid() || end_loc.isInvalid())
//     return false;

//   // if (!src_mgr_.isInFileID(start_loc, src_mgr_.getMainFileID()) &&
//   //     !src_mgr_.isInFileID(end_loc, src_mgr_.getMainFileID()))
//   //   return false;

//   // cout << "MusicASTVisitor cp1\n";
//   // cout << start_loc.printToString(src_mgr_) << endl;
//   // cout << end_loc.printToString(src_mgr_) << endl;
//   // cout << src_mgr_.isInFileID(start_loc, src_mgr_.getMainFileID()) << endl;
//   // cout << src_mgr_.isInFileID(end_loc, src_mgr_.getMainFileID()) << endl;

//   // Skip nodes that are not in the to-be-mutated file.
//   if (src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(start_loc).getHashValue() &&
//       src_mgr_.getMainFileID().getHashValue() != src_mgr_.getFileID(end_loc).getHashValue())
//     return false;

//   // cout << "cp2\n";

//   if (start_loc.isMacroID())
//   {
//     /* THIS PART WORKS FOR CLANG 6.0.1 */
//     // pair<SourceLocation, SourceLocation> expansion_range = 
//     //     rewriter_.getSourceMgr().getImmediateExpansionRange(start_loc);
//     // start_loc = expansion_range.first;
//     /*=================================*/

//     /* THIS PART WORKS FOR CLANG 7.0.1 */
//     CharSourceRange expansion_range = 
//         rewriter_.getSourceMgr().getImmediateExpansionRange(start_loc);
//     start_loc = expansion_range.getBegin();
//     /*=================================*/
//   }

//   // cout << "cp2\n";

//   if (end_loc.isMacroID())
//   {
//     /* THIS PART WORKS FOR CLANG 6.0.1 */
//     // pair<SourceLocation, SourceLocation> expansion_range = 
//     //     rewriter_.getSourceMgr().getImmediateExpansionRange(end_loc);
//     // end_loc = expansion_range.second;
//     /*=================================*/

//     /* THIS PART WORKS FOR CLANG 7.0.1 */
//     CharSourceRange expansion_range = 
//         rewriter_.getSourceMgr().getImmediateExpansionRange(end_loc);
//     end_loc = expansion_range.getEnd();
//     /*=================================*/

//     end_loc = Lexer::getLocForEndOfToken(
//         src_mgr_.getExpansionLoc(end_loc), 0, src_mgr_, comp_inst_->getLangOpts());

//     // Skipping whitespaces (if any)
//     while (*(src_mgr_.getCharacterData(end_loc)) == ' ')
//       end_loc = end_loc.getLocWithOffset(1);

//     // Return if macro is variable-typed. 
//     if (*(src_mgr_.getCharacterData(end_loc)) != '(')
//       return true;

//     // Find the closing bracket of this function-typed macro
//     int parenthesis_counter = 1;
//     end_loc = end_loc.getLocWithOffset(1);

//     while (parenthesis_counter != 0)
//     {
//       if (*(src_mgr_.getCharacterData(end_loc)) == '(')
//         parenthesis_counter++;

//       if (*(src_mgr_.getCharacterData(end_loc)) == ')')
//         parenthesis_counter--;

//       end_loc = end_loc.getLocWithOffset(1);
//     }
//   }
//   // cout << "cp3\n";

//   return true;
// }
