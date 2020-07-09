#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>

#include "clang/AST/PrettyPrinter.h"
#include "clang/Lex/Lexer.h"

#include "lemon_utility.h"

using namespace clang;
using namespace std;

bool directoryExists(const std::string &directory) {
  if(!directory.empty()) {
    if(access(directory.c_str(), 0) == 0) {
      struct stat status;
      stat(directory.c_str(), &status);

      if(status.st_mode & S_IFDIR)
        return true;
    }
  }
  // if any condition fails
  return false;
}

bool fileExists(const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

std::string getCurrentWorkingDir(void) {
  char buff[FILENAME_MAX];
  getcwd(buff, FILENAME_MAX);
  string current_working_dir(buff);
  return current_working_dir;
}

int getLineNumber(clang::SourceManager &src_mgr, clang::SourceLocation loc) {
  return static_cast<int>(src_mgr.getExpansionLineNumber(loc));
}

int getColumnNumber(clang::SourceManager &src_mgr, clang::SourceLocation loc) {
  return static_cast<int>(src_mgr.getExpansionColumnNumber(loc));
}

bool isLocationInTargetFile(
    clang::SourceLocation loc, clang::SourceManager &src_mgr) {
  return src_mgr.getMainFileID().getHashValue() == \
         src_mgr.getFileID(loc).getHashValue();
}

clang::CharSourceRange getMacroUsageRange(
    clang::SourceLocation &loc, clang::SourceManager &src_mgr) {
  return src_mgr.getImmediateExpansionRange(loc);
}

std::string convertStmtToString(
    clang::Stmt *s, clang::CompilerInstance &comp_inst)
{
  /*string SStr;
  llvm::raw_string_ostream S(SStr);
  s->printPretty(S, 0, PrintingPolicy(comp_inst.getLangOpts()));
  return S.str();*/

  SourceLocation start_loc = getPreciseLocation(
      s->getBeginLoc(), comp_inst, true);
  SourceLocation end_loc = getPreciseLocation(
      s->getEndLoc(), comp_inst, false);

  return getCodeInSourceRange(start_loc, end_loc, comp_inst.getSourceManager());
}

std::string getCodeInSourceRange(
    clang::SourceLocation start_loc, clang::SourceLocation end_loc,
    clang::SourceManager &src_mgr) {
  string ret = "";
  clang::SourceLocation temp_loc = start_loc;

  while (temp_loc != end_loc) {
    ret += (*(src_mgr.getCharacterData(temp_loc)));
    temp_loc = temp_loc.getLocWithOffset(1);
  }

  return ret;
}

clang::SourceLocation getPreciseLocation(
    clang::SourceLocation loc, clang::CompilerInstance &comp_inst, bool isStart) {
  SourceManager &src_mgr = comp_inst.getSourceManager();

  // Start location of a Stmt is most of the time correct, except for MACRO.
  // So we handle only the macro case.
  if (isStart && loc.isMacroID()) {
    CharSourceRange expansion_range = getMacroUsageRange(loc, src_mgr);
    return expansion_range.getBegin();
  }

  // End location is usually wrong.
  if (!isStart) {
    // If it is a MACRO, the handle similar to start location.
    if (loc.isMacroID()) {
      CharSourceRange expansion_range = getMacroUsageRange(loc, src_mgr);
      return Lexer::getLocForEndOfToken(
          expansion_range.getEnd(), 0, src_mgr, comp_inst.getLangOpts());
    }
    // Otherwise, use getEndLocOfToken and hope for the best.
    else
      return Lexer::getLocForEndOfToken(
          loc, 0, src_mgr, comp_inst.getLangOpts());
  }

  return loc;
}

bool sourceRangeIsInSameMacro(
    clang::SourceLocation start, clang::SourceLocation end,
    clang::SourceManager &src_mgr) {
  if (!start.isMacroID() || !end.isMacroID())
    return false;

  return src_mgr.getExpansionLoc(start) == src_mgr.getExpansionLoc(end);
}

bool isDeclRefExpr(clang::Stmt *s) {
  if (DeclRefExpr *dre = dyn_cast<DeclRefExpr>(s)) {
    // An Enum value is not a declaration reference
    if (isa<EnumConstantDecl>(dre->getDecl()))
      return false;

    return true;
  }

  return false;
}

bool isPointerDereferenceExpr(clang::Stmt *s) {
  if (UnaryOperator *uo = dyn_cast<UnaryOperator>(s)) {
    if (uo->getOpcode() == UO_Deref)
      return true;

    return false;
  }

  return false;
}

bool isVariableReferenceExpr(clang::Stmt *s) {
  // A Variable Reference Expression is a variable (DeclRefExpr), 
  // member reference (MemberExpr), ArraySubscriptExpr, or Pointer Deference.
  return isDeclRefExpr(s) ||  isPointerDereferenceExpr(s) ||
         isa<ArraySubscriptExpr>(s) || isa<MemberExpr>(s);
}

bool isArithmeticType(clang::Expr *e) {
  return e->getType().getCanonicalType().getTypePtr()->isScalarType() && 
         !e->getType().getCanonicalType().getTypePtr()->isPointerType(); 
}

bool isBooleanType(clang::Expr *e) {
  return e->getType().getCanonicalType().getTypePtr()->isBooleanType();
}

bool isPointerType(Expr *e) {
  return e->getType().getCanonicalType().getTypePtr()->isPointerType();
}

bool isArrayType(Expr *e) {
  return ((e->getType().getCanonicalType()).getTypePtr())->isArrayType(); 
}

std::string getFilenameWithoutLeadingPath(std::string filename) {
  size_t last_slash = filename.find_last_of("/");
  if (last_slash != string::npos)
    return filename.substr(last_slash+1);
  else
    return filename;
}

std::vector<int> splitStringToIntVector(std::string s, std::string delim) {
  size_t pos = 0;
  string token;
  vector<int> ret;

  while ((pos = s.find(delim)) != string::npos) {
    token = s.substr(0, pos);
    cout << token << ", ";
    ret.push_back(stoi(token));
    s.erase(0, pos + delim.length());
  }

  cout << s << endl;
  ret.push_back(stoi(s));
  return ret;
}
