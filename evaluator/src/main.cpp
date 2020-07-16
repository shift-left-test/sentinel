#include <iostream>
#include <tinyxml2.h>
#include <vector>
#include <set>
#include <string>
#include <algorithm>

using namespace std;
using namespace tinyxml2;

void printUsageMsg() {
  cout << "Usage: evaluator <orig result dir> <new result dir>" << endl;
  cout << "Description: compare result files in 2 directories." << endl;
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
    if (testcase->FirstChildElement("failure") != nullptr) 
      fail_testcases.push_back(ts_name + "." + tc_name);

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

  return fail_testcases;
}


int main(int argc, char const *argv[])
{
  if (argc != 3) {
    printUsageMsg();
    return 1;
  }

  vector<string> orig_fail_testcases = findAllFailTestCases(argv[1]);
  cout << "Fail test cases from original test results:" << endl;
  for (auto e: orig_fail_testcases)
    cout << e << endl;
  cout << "===========================================" << endl;

  vector<string> new_fail_testcases = findAllFailTestCases(argv[2]);
  cout << "Fail test cases from new test results:" << endl;
  for (auto e: new_fail_testcases)
    cout << e << endl;
  cout << "===========================================" << endl;  

  vector<string> union_set, intersection_set, killing_testcases;
  set_union(orig_fail_testcases.begin(), orig_fail_testcases.end(),
            new_fail_testcases.begin(), new_fail_testcases.end(),
            inserter(union_set, union_set.begin()));
  set_intersection(orig_fail_testcases.begin(), orig_fail_testcases.end(),
                   new_fail_testcases.begin(), new_fail_testcases.end(),
                   inserter(intersection_set, intersection_set.begin()));
  set_difference(union_set.begin(), union_set.end(),
                 intersection_set.begin(), intersection_set.end(),
                 inserter(killing_testcases, killing_testcases.begin()));

  cout << "Test cases killing this mutant:" << endl;
  for (auto e: killing_testcases)
    cout << e << endl;
  cout << "===========================================" << endl;

  if (killing_testcases.size() == 0)
    cout << "MUTANT SURVIVED!" << endl;
  else
    cout << "MUTANT IS KILLED!" << endl;

  // The mutant is killed if 
  // (1) numbers of fail testcases, or 
  // (2) orig_fail_testcases and new_fail_testcases contain different elements.
  // if (orig_fail_testcases.size() == new_fail_testcases.size())
  //   killed = true;
  // else {
  //   sort(orig_fail_testcases.begin(), orig_fail_testcases.end());
  //   sort(new_fail_testcases.begin(), new_fail_testcases.end());
    
  //   for (int i = 0; i < orig_fail_testcases.size(); i++)
  //     if (orig_fail_testcases[i].compare(new_fail_testcases) != 0) {
  //       killed = true;
  //       break;
  //     }
  // }

  return 0;
}
