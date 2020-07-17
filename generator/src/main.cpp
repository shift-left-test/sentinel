#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <stdexcept>
#include <cstdlib>

using namespace std;

class Mutant
{
public:
  Mutant(string info) {
    // Get original/target filename
    size_t idx = info.find(',');
    orig_filename = info.substr(0, idx);
    mutant_filename = orig_filename + ".mutant";
    info = info.substr(idx+1);
    
    // Get mutation oeprator
    idx = info.find(',');
    mutation_operator = info.substr(0, idx);
    info = info.substr(idx+1);

    // Get target mutation location (start/end line/col)
    idx = info.find(',');
    start_line = stoi(info.substr(0, idx));
    info = info.substr(idx+1);

    idx = info.find(',');
    start_col = stoi(info.substr(0, idx));
    info = info.substr(idx+1);

    idx = info.find(',');
    end_line = stoi(info.substr(0, idx));
    info = info.substr(idx+1);

    idx = info.find(',');
    end_col = stoi(info.substr(0, idx));
    info = info.substr(idx+1);

    // Get target token.
    // Check if target_token is enclosed in double quotes.
    // If not, assign it to the substring upto the next comma.
    // Otherwise, identify double quote pair and assign to string in between.
    // NOTE: Target token is not really needed for mutant generation since we
    // already have the target mutation location. But it is in the database,
    // in case anyone needs it. Can be removed if necessary.
    if (info[0] != '\"') {
      idx = info.find(',');
      target_token = info.substr(0, idx);
      mutated_token = info.substr(idx+1);
      return;
    }
    else {
      // Find the closing double quote.
      idx = 1;
      while (true) {
        if (info[idx] == '\"') {
          if (info[idx+1] == '\"') { 
            // escaped double quote
            target_token += '\"';
            idx += 2;
          }
          else { 
            // closing double quote. point idx to the comma after double quote.
            idx += 1;
            break;
          }
        }
        else {
          target_token += info[idx];
          idx += 1;
        }
      }
    }

    // Get mutated token
    // Handle empty string case, escaped quoted case.
    if (info.length() == 0 || idx + 1 == info.length())
      mutated_token = "";
    else {
      info = info.substr(idx+1);
      if (info[0] == '\"') {
        mutated_token = info.substr(1, info.length()-1);
        size_t idx = info.find("\"\"");
        while (idx != string::npos) {
          mutated_token = mutated_token.substr(0, idx) + mutated_token.substr(idx+2);
          idx = mutated_token.find("\"\"");
        }
      }
      else
        mutated_token = info;
    }
  }

  ~Mutant() {}
  
  string orig_filename;
  string mutant_filename;
  string mutation_operator;
  int start_line;
  int start_col;
  int end_line;
  int end_col;
  string target_token;
  string mutated_token;
};

void printUsageMsg() {
  cout << "Usage: generator <mutant info> -<b/r>" << endl;
  cout << "Description: generate mutant using <mutant info>." << endl;
  cout << "             if -b is given, the original file is backed up.\n";
  cout << "             if -r is given, the backup version is copied back to ";
  cout << "its original location." << endl;
}

bool fileExists(const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

int parseLineNum(const char *input_num) {
  string arg{input_num};
  try {
    size_t pos;
    int x = stoi(arg, &pos);

    if (pos < arg.size()) {
      cerr << "Warning: Trailing characters after number: " << arg << '\n';
    }

    if (x < 1) {
      cerr << "Number must be a positive integer larger than 0." << endl;
      exit(1);
    }

    return x; 
  } catch (invalid_argument const &ex) {
    cerr << "Invalid Number: " << arg << '\n';
    exit(1);
  } catch (out_of_range const &ex) {
    cerr << "Number out of range: " << arg << '\n';
    exit(1);
  }
}

string getTargetLine(string mutant_filename, int line_num) {
  ifstream mutant_file(mutant_filename);
  string line;
  int line_idx = 0;

  // Handle escaped newline (...,"...\n...",...).
  // File reader sees as 2 lines, but we want this as 1 line.
  // For each line, count number of double quotes.
  // If it is odd, append next line into currently parsed line.
  while (getline(mutant_file, line)) {
    line_idx += 1;
    
    string temp = line;
    int dquote_counter = 0;
    while (true) {      
      for (auto c: temp) {
        if (c == '\"')
          dquote_counter += 1;
      }

      if (dquote_counter % 2 == 0)
        break;

      getline(mutant_file, temp);
      line = line + '\n' + temp;
    }

    if (line_idx == line_num) {
      mutant_file.close();
      return line;
    }
  }

  mutant_file.close();
  cerr << "Target mutant does not exist" << endl;
  exit(1);
}

void generateMutant(string info, string option) {
  if (info.size() == 0) {
    cerr << "No information specified for target mutant." << endl;
    exit(1);
  }

  Mutant m{info};

  string backup_filename{m.orig_filename+".orig"};
  string command;
  // If -b is given, then backup file is made by creating a copy of the original
  // file with .orig appended to original filename.
  // If -r is given, move the backup file back to its original location.
  // If the file does not exist, 
  if (option.compare("-b") == 0) {
    cout << "Creating backup: " << backup_filename << endl;
    command = "cp " + m.orig_filename + " " + backup_filename;
    system(command.c_str());
  }
  else {
    if (!fileExists(backup_filename)) {
      cerr << "Backup file does not exist: " << backup_filename << endl;
      exit(1);
    }

    cout << "Original file is recovered: " << m.orig_filename << endl;
    command = "mv " + backup_filename + " " + m.orig_filename;
    system(command.c_str());
    return;
  }

  cout << "Mutating " << m.orig_filename << endl;
  // cout << "Genercd ..ating mutant file: " << m.mutant_filename << endl;
  printf("From line %d col %d to line %d col %d\n", m.start_line, m.start_col, 
                                                    m.end_line, m.end_col);
  printf("Mutating \"%s\" to \"%s\"\n", m.target_token.c_str(), 
                                        m.mutated_token.c_str());

  ifstream orig_file(m.orig_filename);
  ofstream mutant_file(m.mutant_filename, ios::trunc);

  // If code line is out of target range, just write to mutant file.
  // If code line is in target range (start_line < code_line < end_line), skip.
  // If code line is on start_line, write the code appearing before start_col,
  // and write mutated token.
  // If code line is on end_line, write the code appearing after end_col.
  string line;
  int line_idx = 0;

  while (getline(orig_file, line)) {
    line_idx += 1;

    if (line_idx < m.start_line or line_idx > m.end_line)
      mutant_file << line << endl;

    if (line_idx == m.start_line) {
      mutant_file << line.substr(0, m.start_col-1);
      mutant_file << m.mutated_token;
    }      

    if (line_idx == m.end_line) 
      mutant_file << line.substr(m.end_col-1) << endl;
  }

  orig_file.close();
  mutant_file.close();

  command = "mv " + m.mutant_filename + " " + m.orig_filename;
  system(command.c_str()); 
}

int main(int argc, char const **argv)
{
  if (argc < 3) {
    printUsageMsg();
    return 1;
  }

  // Check if input csv file exists
  // string mutant_filename{argv[1]};
  // if (!fileExists(mutant_filename)) {
  //   cerr << "Input <mutants.csv> file does not exist: " << mutant_filename << endl;
  //   exit(1);
  // }

  // Check if line number is a positive integer larger than 1.
  // int line_num = parseLineNum(argv[2]);

  // Get target line. 
  // Exit if line is empty. (line number may be larger than file length)
  // string target_line = getTargetLine(mutant_filename, line_num);
  
  string target_line = argv[1];
  string option = argv[2];

  // Generate mutant based on target line
  generateMutant(target_line, option);

  return 0;
}