#ifndef LEMON_MUTANT_DATABASE_H_
#define LEMON_MUTANT_DATABASE_H_

#include <string>
#include <map>
#include <vector>
#include <iostream>

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "mutant_entry.h"
#include "lemon_configuration.h"

typedef int LineNumber;
typedef std::vector<MutantEntry> MutantEntryList;
typedef std::map<LineNumber, MutantEntryList> MutantEntryTable;

class MutantDatabase
{
public:
  MutantDatabase(clang::CompilerInstance& comp_inst, Configuration* config);

  void addMutantEntry(
      std::string operator_name, clang::SourceLocation start_loc,
      clang::SourceLocation end_loc, std::string targeted_token,
      std::string mutated_token, int target_line);

  void writeMutantToDatabaseFile(const MutantEntry &entry);
  void generateMutantSrcFile(const MutantEntry &entry);
  void generateToolOutput();

  const MutantEntryTable& getEntryTable() const;
  clang::CompilerInstance& getCompilerInstance();
  Configuration *getConfiguration();

private:
  clang::CompilerInstance& CI;
  clang::SourceManager &src_mgr;
  clang::LangOptions &lang_opts;
  clang::Rewriter rewriter_;

  MutantEntryTable mutant_entry_table;
  Configuration *config;

  // std::string input_filename_;
  // std::string database_filename_;
  // std::string output_dir_;
  int mutant_id;

  // maxi number of mutants generated per mutation point per mutation operator
  // int num_mutant_limit_;

  std::string getNextMutantFilename();
  void incrementNextMutantfileId();
  // bool validateSourceRange(
  //     clang::SourceLocation &start_loc, clang::SourceLocation &end_loc);
};

#endif  // LEMON_MUTANT_DATABASE_H_