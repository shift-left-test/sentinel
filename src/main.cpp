#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <limits>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"

#include "lemon_ast_consumer.h"
#include "lemon_utility.h"
#include "mutant_database.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;
using namespace std;

static llvm::cl::OptionCategory LemonCategory("LEMON options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
// static cl::extrahelp MoreHelp("\nMore help text...\n");

static llvm::cl::opt<string> select_output_dir(
    "o",
    llvm::cl::desc("Select output directory."),
    llvm::cl::value_desc("output_dir"),
    llvm::cl::Optional,
    llvm::cl::cat(LemonCategory)
);

// static llvm::cl::list<string> OptionM(
//     "m", llvm::cl::desc("Specify mutant operator name to use"), 
//     llvm::cl::value_desc("mutator_name"),
//     llvm::cl::cat(MusicOptions));
static llvm::cl::list<Mutators> mutator_list(
  llvm::cl::desc("Available Mutation Operators: "),
  llvm::cl::values(
    clEnumVal(aor, "Arithmetic Operator Replacement"),
    clEnumVal(bor, "Bitwise Operator Replacement"),
    clEnumVal(lcr, "Logical Connector Replacement"),
    clEnumVal(ror, "Relational Operator Replacement"),
    clEnumVal(sbr, "Statement Break Replacement"),
    clEnumVal(sor, "Shift Operator Replacement"),
    clEnumVal(uoi, "Unary Operator Insertion")    
  ),
  llvm::cl::cat(LemonCategory)
);

static llvm::cl::opt<string> target_filename(
    "target",
    llvm::cl::desc("Specify file contain targeted lines of each input file."),
    llvm::cl::value_desc("filename"),
    llvm::cl::Optional,
    llvm::cl::cat(LemonCategory)
);

static llvm::cl::opt<bool> debug_on(
    "debug_on",
    llvm::cl::desc("Print out messages for debugging purposes."),
    llvm::cl::Optional,
    llvm::cl::cat(LemonCategory)
);

static llvm::cl::opt<bool> do_generate_mutants(
    "do_generate_mutants",
    llvm::cl::desc("Trigger mutant generation."),
    llvm::cl::Optional,
    llvm::cl::cat(LemonCategory)
);

string g_output_dir = "./";
FilenameToLineMap g_filename_to_line_map;
vector<Mutators> g_selected_mutators;

Configuration *g_config;
MutantDatabase *g_database;

void ParseOptionO()
{
  // Parse option -o (if provided)
  // Terminate tool if given output directory does not exist.
  if (!select_output_dir.empty())
  {
    if (directoryExists(select_output_dir))
      g_output_dir = select_output_dir;
    else
    {
      cout << "Invalid directory for -o option: " << select_output_dir << endl;
      exit(1);
    }
  }

  // In case user input does not end with slash (/).
  // Just to make it easier to add filename later.
  if (g_output_dir.back() != '/')
    g_output_dir += "/";

  cout << "Output Directory: " << g_output_dir << "\n";
}

void ParseMutatorList()
{
  if (mutator_list.size() == 0) {
    g_selected_mutators = {aor, bor, lcr, ror, sbr, sor, uoi, uor};
    return;
  }

  for (unsigned i = 0; i != mutator_list.size(); ++i)
    g_selected_mutators.push_back(mutator_list[i]);
}

void ParseTargetFilename()
{
  if (target_filename.empty()) return;
  if (!fileExists(target_filename)) {
    cerr << "Invalid filename for -target: " << select_output_dir << endl;
    exit(1);
  }

  // Save data from target_file to g_filename_to_line_map
  ifstream target_file(target_filename);
  string line;

  while (getline(target_file, line)) {
    rtrim(line);
    if (line.length() == 0) continue;

    size_t colon_loc = line.find_last_of(":");
    if (colon_loc == string::npos) {
      cerr << "Target file format is wrong" << endl;
      exit(1);
    }

    string filename = getFilenameWithoutLeadingPath(line.substr(0, colon_loc));
    cout << "Target lines for " << filename << ": ";
    g_filename_to_line_map[filename] = splitStringToIntVector(line.substr(colon_loc+1), ",");
  }
}

void ParseDebugOption()
{
  return;
}

class GenerateMutant : public ASTFrontendAction
{
public:
  virtual unique_ptr<ASTConsumer> CreateASTConsumer(
      CompilerInstance &CI, llvm::StringRef InFile) {
    // Disable target file compilation message (warnings or errors).
    // CI.getDiagnostics().setClient(new IgnoringDiagConsumer());

    g_config = new Configuration(InFile.str(), &g_filename_to_line_map,
                                 debug_on, g_selected_mutators, g_output_dir);
    g_database = new MutantDatabase(CI, g_config);
    cout << "---------------------------------------\n";

    return unique_ptr<ASTConsumer>(new LemonASTConsumer(CI, g_database));
  }

protected:
  void ExecuteAction() override {
    CompilerInstance &CI = getCompilerInstance();
    CI.getPreprocessor().createPreprocessingRecord();
    
    cout << "Mutating " << g_config->getInputFilename() << endl;
    ASTFrontendAction::ExecuteAction();
    cout << "Mutation Complete!\n";

    if (do_generate_mutants) {
      string database_filename = g_config->getMutationDbFilename();
      checkDatabaseFileExists();

      // Open the file with mode TRUNC to create the file if not existed. 
      ofstream database_file(database_filename.data(), ios::trunc);
      if (!database_file.is_open()) {
        std::cerr << "Failed to open file : " << strerror(errno) << std::endl;
        exit(1);
      }

      // database_file << "Orignal Filename,Mutant Name,Mutation Operator,";
      // database_file << "Start Line#,Start Col#,Target Token,Mutated Token" << endl;
      // database_file << "{" << endl;
      // database_file << "  \"filename\":\"" << g_config->getInputFilenameWithPath() << "\"," << endl;
      // database_file << "  \"mutants\": [\n    {" << endl;
      database_file.close();

      g_database->generateToolOutput();
    }    
    
    g_database->writeMutantTargetsToFile();

    // ofstream database_file(database_filename.data(), ios::trunc);
    // database_file << "  ]\n}" << endl;
    // database_file.close();
    cout << "=======================================\n";
  }

private:
  void checkDatabaseFileExists() {
    // Check if there is any file with same name as database file.
    string database_filename = g_config->getMutationDbFilename();
    while (fileExists(database_filename)) {
      // If there is, ask the user if they want to keep the file
      cout << "File " << database_filename << " already exists\n";
      string keepFile = "";
      while (keepFile.compare("y") != 0 && keepFile.compare("n") != 0) {
        cout << "Do you want to keep the file (y/n)? ";
        keepFile = "";
        cin >> keepFile;
      }

      if (keepFile.compare("y") == 0 || keepFile.compare("Y") == 0) {
        cout << "Press Enter after you have moved the file.";
        // First one to flush the newline from above cin. 
        std::cin.ignore(std::numeric_limits <std::streamsize>::max(), '\n');
        // Second to wait.
        std::cin.ignore(std::numeric_limits <std::streamsize>::max(), '\n');
      }
      else 
        break;
    }
  }
};

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, LemonCategory);

  ParseMutatorList();
  ParseOptionO();
  ParseTargetFilename();
  ParseDebugOption();

  for (auto file: op.getSourcePathList()) {
    if (op.getCompilations().getCompileCommands(file).size() > 1)
    {
      cout << file << " has more than 1 compile commands\n" << endl;

      for (auto e: op.getCompilations().getCompileCommands(file))
      {
        for (auto command: e.CommandLine)
          cout << command << " ";
        cout << endl;
      } 

      // getchar();
      continue;
    }
  }

  ClangTool Tool(op.getCompilations(), op.getSourcePathList());
  // process command line option
  return Tool.run(newFrontendActionFactory<GenerateMutant>().get());
}
