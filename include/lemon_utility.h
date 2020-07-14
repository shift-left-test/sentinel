#ifndef LEMON_UTILITY_H_
#define LEMON_UTILITY_H_

#include <string>
#include <algorithm> 
#include <cctype>
#include <locale>
#include <vector>

#include "clang/Basic/SourceManager.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Expr.h"

bool directoryExists(const std::string &directory);
bool fileExists(const std::string& name);
std::string GetCurrentWorkingDir(void);
int getLineNumber(clang::SourceManager &src_mgr, clang::SourceLocation loc);
int getColumnNumber(clang::SourceManager &src_mgr, clang::SourceLocation loc);
bool isLocationInTargetFile(
    clang::SourceLocation loc, clang::SourceManager &src_mgr);
clang::CharSourceRange getMacroUsageRange(
    clang::SourceLocation &loc, clang::SourceManager &src_mgr);
std::string convertStmtToString(
    clang::Stmt *s, clang::CompilerInstance &comp_inst);
std::string getCodeInSourceRange(
    clang::SourceLocation start_loc, clang::SourceLocation end_loc,
    clang::SourceManager &src_mgr);
clang::SourceLocation getPreciseLocation(
    clang::SourceLocation loc, clang::CompilerInstance &comp_inst, bool isStart);
bool sourceRangeIsInSameMacro(
    clang::SourceLocation start, clang::SourceLocation end,
    clang::SourceManager &src_mgr);
bool isDeclRefExpr(clang::Stmt *s);
bool isPointerDereferenceExpr(clang::Stmt *s);
bool isVariableReferenceExpr(clang::Stmt *s);
bool isArithmeticType(clang::Expr *e);
bool isBooleanType(clang::Expr *e);
bool isPointerType(clang::Expr *e);
bool isArrayType(clang::Expr *e);
std::string getFilenameWithoutLeadingPath(std::string filename);
std::vector<int> splitStringToIntVector(std::string s, std::string delim);
void SplitStringIntoVector(std::string target, 
                           std::vector<std::string> &out_vector, 
                           const std::string delimiter);
bool IsAllDigits(const std::string s);

// trim from start (in place)
static inline void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
    return !std::isspace(ch);
  }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}

#endif  // LEMON_UTILITY_H_
