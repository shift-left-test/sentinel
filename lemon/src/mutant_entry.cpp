#include "mutant_entry.h"

MutantEntry::MutantEntry(
    std::string operator_name, std::string token, std::string mutated_token,
    clang::SourceLocation start_location, clang::SourceLocation end_location)
  : operator_name(operator_name), targeted_token(token),
    mutated_token(mutated_token), start_loc(start_location),
    end_loc(end_location) {

}

std::string MutantEntry::getOperatorName() const {
  return operator_name;
}

std::string MutantEntry::getTargetedToken() const {
  return targeted_token;
}
std::string MutantEntry::getMutatedToken() const {
  return mutated_token;
}

clang::SourceLocation MutantEntry::getStartLocation() const {
  return start_loc;
}
clang::SourceLocation MutantEntry::getEndLocation() const {
  return end_loc;
}

bool MutantEntry::operator==(const MutantEntry &other) const {
  return start_loc == other.getStartLocation() && 
         targeted_token.compare(other.getTargetedToken()) == 0 &&
         mutated_token.compare(other.getMutatedToken()) == 0;
}
