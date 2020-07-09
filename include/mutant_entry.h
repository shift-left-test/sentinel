#ifndef LEMON_MUTANT_ENTRY_H_
#define LEMON_MUTANT_ENTRY_H_ 

#include <string>
#include <iostream>
#include <vector>

#include "clang/Basic/SourceLocation.h"
// #include "clang/Basic/SourceManager.h"

class MutantEntry
{
public:
  // clang::SourceManager &src_mgr_;

  MutantEntry(
      std::string operator_name, std::string token, std::string mutated_token,
      clang::SourceLocation start_location, clang::SourceLocation end_location);

  // getters
  std::string getOperatorName() const;
  std::string getTargetedToken() const;
  std::string getMutatedToken() const;

  // only end location changes after mutation
  clang::SourceLocation getStartLocation() const;
  clang::SourceLocation getEndLocation() const;

  bool operator==(const MutantEntry &other) const;

private:
  std::string operator_name;
  std::string targeted_token;
  std::string mutated_token;
  clang::SourceLocation start_loc;
  clang::SourceLocation end_loc;
};

#endif  // LEMON_MUTANT_ENTRY_H_