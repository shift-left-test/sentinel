#include <iostream>
#include <tinyxml2.h>
#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <dirent.h>
# include <sys/stat.h>

using namespace std;
using namespace tinyxml2;

void printUsageMsg() {
  cout << "Usage: evaluator <orig result dir> <new result dir>" << endl;
  cout << "Description: compare result files in 2 directories." << endl;
}

bool isDirectory(string path) {
  struct stat path_stat;
  stat(path.c_str(), &path_stat);
  return S_ISDIR(path_stat.st_mode);
}

vector<string> findFailTestCasesFromTestsuite(XMLElement *testsuite) {
  int failures;
  testsuite->QueryIntAttribute("failures", &failures);
  if (failures == 0) {
    testsuite = testsuite->NextSiblingElement("testsuite");
    return vector<string>();
  }

  vector<string> fail_testcases;
  string ts_name = (string) testsuite->Attribute("name");
  XMLElement *testcase = testsuite->FirstChildElement("testcase");
  while (testcase != nullptr) {
    string tc_name = (string) testcase->Attribute("name");

    // A fail test case in GoogleTest contain <failure> child element.
    if (testcase->FirstChildElement("failure") != nullptr) {
      fail_testcases.push_back(ts_name + "." + tc_name);
      testcase = testcase->NextSiblingElement("testcase");
      continue;
    }

    // A fail test case in QT contain result attribute with value "fail"
    const char * result_attr = testcase->Attribute("result");   
    if (result_attr != nullptr) {
      string result = result_attr;
      if (result.compare("fail") == 0)
        fail_testcases.push_back(ts_name + "." + tc_name);
    }     

    testcase = testcase->NextSiblingElement("testcase");
  }

  return fail_testcases;
}

vector<string> findAllFailTestCases(string filename) {
  XMLDocument doc;
  XMLError eResult;
  doc.LoadFile(filename.c_str());

  // If test result file contains multiple testsuites, find <testsuites>.
  // Otherwise, seult file starts with <testsuite>.
  // If none of those exists, exits with error message.
  XMLElement *pRoot = doc.FirstChildElement("testsuites");
  if (pRoot == nullptr) {
    pRoot = doc.FirstChildElement("testsuite");
    if (pRoot== nullptr) {
      cerr << "Cannot find starting <testsuite> or <testsuites> element in "
           << filename << endl;
      exit(1);
    }

    return findFailTestCasesFromTestsuite(pRoot);
  }

  // If number of failures is 0 then return empty list.
  int total_failures;
  pRoot->QueryIntAttribute("failures", &total_failures);
  if (total_failures == 0)
    return vector<string>();
  
  // Find test suite with number of failures > 0.
  // Find test case within such test suite with <failure> element.
  // Store the name of such test case in set fail_testcases.
  XMLElement *testsuite = pRoot->FirstChildElement("testsuite");
  vector<string> fail_testcases;
  while (testsuite != nullptr) {
    vector<string> ts_fail_testcases = findFailTestCasesFromTestsuite(testsuite);
    for (auto e: ts_fail_testcases)
      fail_testcases.push_back(e);

    testsuite = testsuite->NextSiblingElement("testsuite");
  }

  sort(fail_testcases.begin(), fail_testcases.end());

  return fail_testcases;
}

vector<string> getListOfResultFiles(string path, string travel_path) {
  DIR *dir;
  struct dirent *ent;
  vector<string> result_files;
  string result_path = path + "/" + travel_path;

  // Traverse each file and directory in give path.
  // For file, add to result_files if it has xml ending.
  // For directory, recursively traverse with updated path and travel path.
  // Travel path = relative path with respect to path
  if ((dir = opendir(result_path.c_str())) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      string name = ent->d_name;
      if (name.compare(".") == 0 || name.compare("..") == 0)
        continue;

      if (isDirectory(path+"/"+travel_path+name)) {
        vector<string> temp = getListOfResultFiles(path, travel_path+name+"/");
        result_files.insert(result_files.end(), temp.begin(), temp.end());
      }
      else {
        // Anything smaller than a.xml is not considered
        if (name.length() < 5) continue;
        if (name.substr(name.length()-4,4).compare(".xml") == 0) {
          result_files.push_back(travel_path+name);
        }
      }
    }

    closedir (dir);
    sort(result_files.begin(), result_files.end());
    return result_files;
  } else {
    /* could not open directory */
    cout << "Warning: cannot open directory " << path << endl;
    return vector<string>();
  }
}

set<string> set_union(vector<string> s1, vector<string> s2) {
  set<string> ret;

  for (string e: s1)
    if (ret.find(e) == ret.end())
      ret.insert(e);

  for (string e: s2)
    if (ret.find(e) == ret.end())
      ret.insert(e);

  return ret;    
}

set<string> set_intersection(vector<string> s1, vector<string> s2) {
  set<string> ret; 

  for (string e1: s1) 
    for (string e2: s2)
      if (e1.compare(e2) == 0) {
        ret.insert(e1);
        break;
      }

  return ret;
}

vector<string> set_difference(set<string> s1, set<string> s2) {
  vector<string> ret;

  for (string e: s1)
    if (s2.find(e) == s2.end())
      ret.push_back(e);

  return ret;
}

vector<string> getKillingTestcases(string filename1, string filename2) {
  vector<string> orig_fail_testcases = findAllFailTestCases(filename1);
  // cout << "Fail test cases from " << filename1 << ":" << endl;
  // for (auto e: orig_fail_testcases)
  //   cout << e << endl;
  // cout << "===========================================" << endl;

  vector<string> new_fail_testcases = findAllFailTestCases(filename2);
  // cout << "Fail test cases from " << filename2 << ":" << endl;
  // for (auto e: new_fail_testcases)
  //   cout << e << endl;
  // cout << "===========================================" << endl;  

  set<string> union_set, intersection_set;
  union_set = set_union(orig_fail_testcases, new_fail_testcases);
  // cout << "union:" << endl;
  // for (auto e: union_set)
  //   cout << e << endl;
  // cout << "===========================================" << endl;

  intersection_set = set_intersection(orig_fail_testcases, new_fail_testcases);
  // cout << "intersection:" << endl;
  // for (auto e: intersection_set)
  //   cout << e << endl;
  // cout << "===========================================" << endl;

  vector<string> killing_testcases = set_difference(union_set, intersection_set);
  // cout << "Fail test cases difference:" << endl;
  // for (auto e: killing_testcases)
  //   cout << e << endl;
  // cout << "===========================================" << endl;

  return killing_testcases;
}

int main(int argc, char const *argv[])
{
  if (argc != 3) {
    printUsageMsg();
    return 1;
  }

  // Get the sorted list of .xml result files from 2 given directories
  string orig_dir = argv[1];
  vector<string> orig_result_files = getListOfResultFiles(orig_dir, "");
  // for (auto e: orig_result_files)
  //   cout << e << endl;

  string new_dir = argv[2];
  vector<string> new_result_files = getListOfResultFiles(new_dir, "");
  // for (auto e: new_result_files)
  //   cout << e << endl;  

  if (orig_result_files.size() != new_result_files.size()) {
    // cout << "Numbers of generated result files are different" << endl;
    cout << "MUTANT IS KILLED!" << endl;
    return 0;
  }

  bool is_killed = false;

  // Check if the result folders contain different files.
  for (int i = 0; i < orig_result_files.size(); i++)
    if (orig_result_files[i].compare(new_result_files[i]) != 0) {
      cout << "MUTANT IS KILLED!" << endl;
      return 0;
    }

  // return 0;
  vector<string> killing_testcases;
  for (auto e: orig_result_files) {
    vector<string> temp = getKillingTestcases(orig_dir+"/"+e,
                                              new_dir+"/"+e);
    killing_testcases.insert(killing_testcases.begin(), temp.begin(), 
                             temp.end());
  }
  

  if (killing_testcases.size() == 0)
    cout << "MUTANT SURVIVED!" << endl;
  else {
    cout << "MUTANT IS KILLED!" << endl;    
    cout << "Test cases killing this mutant:" << endl;
    for (auto e: killing_testcases)
      cout << e << endl;
    cout << "===========================================" << endl;
  }

  return 0;
}
