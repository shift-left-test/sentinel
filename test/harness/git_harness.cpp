/*
  MIT License

  Copyright (c) 2020 Sung Gon Kim

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

// to use assertion
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>

#include "git_harness.hpp"

// Create a folder and git init.
// By default, git repo /temp/temp_repo is created.
GitHarness& GitHarness::initiateGitRepo(std::string dir_name) {
  // Create the directory.
  // Exit with error message if it already existed or fail to generate
  assert(!directoryExists(dir_name));
  executeSystemCommand("mkdir "+dir_name, "Fail to make directory: "+dir_name);

  // git init
  if (git_repository_init(&repo, dir_name.c_str(), false)) {
    std::cerr << "git init fails." << std::endl;
    exit(1);
  }

  repo_path = dir_name;
  return *this;
}

// Create a folder within git directory.
// The folder path should be relative w.r.t. repo path
GitHarness& GitHarness::addFolder(std::string folder_name) {
  assert(repo != nullptr);
  assert(!directoryExists(repo_path+"/"+folder_name));
  folder_name = repo_path + "/" + folder_name;
  executeSystemCommand("mkdir "+folder_name,
                       "Fail to make directory: "+folder_name);

  return *this;
}

// Create a file at specified location (relative path w.r.t repo path)
GitHarness& GitHarness::addFile(std::string filename, std::string content) {
  assert(repo != nullptr);
  assert(!fileExists(repo_path+"/"+filename));
  filename = repo_path + "/" + filename;

  executeSystemCommand("touch "+filename,
                       "Fail to make file: "+filename);

  std::ofstream new_file(filename.c_str(), std::ios::app);
  new_file << content;
  new_file.close();
  return *this;
}

// Add code to file.
// If line number is negative or exceed file length, the code is appended to
// end of file.
// By default, code is appended to end of file.
GitHarness& GitHarness::addCode(std::string filename, std::string &content,
                                int line, int col) {
  assert(repo != nullptr);

  filename = repo_path + "/" + filename;
  assert(fileExists(filename));

  // Create a temporary file to modify file content.
  // Temporary filename is filename.temp
  std::ifstream orig_file(filename);
  std::ofstream changed_file(filename+".temp");

  int line_idx = 0;
  std::string line_str;
  while (std::getline(orig_file, line_str)) {
    line_idx += 1;

    // For non-target line, just copy directly to temporary file
    if (line_idx < line || line_idx > line) {
      changed_file << line_str << std::endl;
      continue;
    }

    // Column number starts from 1.
    // For target line, if column number is negative, content is added to
    // end of line.
    if (col <= 0 || col > line_str.length())
      changed_file << line_str << content << std::endl;
    else
      changed_file << line_str.substr(0, col-1) << content
                   << line_str.substr(col-1, std::string::npos) << std::endl;
  }

  // If line number is negative or exceeding length of file, content is added
  // to end of file.
  if (line <= 0 || line > line_idx)
    changed_file << content;

  orig_file.close();
  changed_file.close();

  // replace original file with changed file.
  executeSystemCommand("mv " + filename + ".temp " + filename,
                       "Fail to replace original file with temp file.");
  return *this;
}

// Delete code between (start_line, start_col) and (end_line, end_col).
// A negative line number indicates end of file.
// A negative column number indicates end of line.
/*GitHarness& GitHarness::deleteCode(std::string &filename, int start_line,
                                   int end_line, int start_col, int end_col) {
  assert(repo != nullptr);
  assert(fileExists(filename));
  return *this;
}*/

// Delete multiple line of codes. Code line starts from 1.
GitHarness& GitHarness::deleteCode(std::string filename,
                                   std::vector<int> lines) {
  assert(repo != nullptr);
  filename = repo_path + "/" + filename;
  assert(fileExists(filename));

  // Create a temporary file to modify file content.
  // Temporary filename is filename.temp
  std::ifstream orig_file(filename);
  std::ofstream changed_file(filename+".temp");

  int line_idx = 0;
  std::string line_str;
  while (std::getline(orig_file, line_str)) {
    line_idx += 1;

    // If line_idx is in vector lines, then the line is skipped.
    // Otherwise, the line is added to temp file.
    if (std::find(lines.begin(), lines.end(), line_idx) != lines.end())
      continue;
    else
      changed_file << line_str << std::endl;
  }

  orig_file.close();
  changed_file.close();

  // replace original file with changed file.
  executeSystemCommand("mv " + filename + ".temp " + filename,
                       "Fail to replace original file with temp file.");
  return *this;
}

// git add <filenames>
GitHarness& GitHarness::stageFile(std::vector<std::string> filenames) {
  assert(repo != nullptr);
  for (int i = 0; i < filenames.size(); i++)
    assert(fileExists(repo_path + "/" + filenames[i]));

  git_index_matched_path_cb matched_cb = NULL;
  git_index *index;
  std::vector<char*> argv;
  convertToCharArray(&filenames, &argv);

  git_strarray array = {argv.data(), argv.size()};
  struct index_options options = {0};
  options.mode = INDEX_ADD;

  /* Grab the repository's index. */
  libgitErrorCheck(git_repository_index(&index, repo),
                   "Could not open repository index");

  options.repo = repo;

  /* Perform the requested action with the index and files */
  git_index_add_all(index, &array, 0, matched_cb, &options);

  /* Cleanup memory */
  git_index_write(index);
  git_index_free(index);

  return *this;
}

// git commit -m "messasge"
GitHarness& GitHarness::commit(std::string message) {
  assert(repo != nullptr);

  git_oid new_commit_id, tree_oid;
  git_tree *tree;
  git_index *index;
  git_object *parent = nullptr;
  git_reference *ref = nullptr;
  git_signature *signature;

  int error = git_revparse_ext(&parent, &ref, repo, "HEAD");
  if (error == GIT_ENOTFOUND) {
    std::cerr << "HEAD not found. Creating first commit\n";
    error = 0;
  } else if (error != 0) {
    std::cerr << "git commit error\n";
  }

  libgitErrorCheck(git_repository_index(&index, repo),
                   "Could not open repository index");
  libgitErrorCheck(git_index_write_tree(&tree_oid, index),
                   "Could not write tree");
  libgitErrorCheck(git_index_write(index), "Could not write index");
  libgitErrorCheck(git_tree_lookup(&tree, repo, &tree_oid),
                   "Error looking up tree");
  libgitErrorCheck(git_signature_now(&signature, "Loc Phan",
                                     "loc.phan@lge.com"),
                   "Error creating signature");

  libgitErrorCheck(git_commit_create_v(&new_commit_id, repo, "HEAD", signature,
                                       signature, "UTF-8", message.c_str(),
                                       tree, parent ? 1 : 0, parent),
                   "Error creating commit");
  commit_ids.push_back(new_commit_id);

  git_index_free(index);
  git_signature_free(signature);
  git_tree_free(tree);

  return *this;
}

// git tag -a <tag_name> -m "message"
/*GitHarness& GitHarness::addTag(std::string tag_name, std::string commit_id,
                               std::string message) {
  assert(repo != nullptr);
  assert(tag_name.length() > 0);

  git_signature *tagger;
  git_oid oid;
  git_object *target;

  libgitErrorCheck(git_revparse_single(&target, repo, commit_id.c_str()),
                   "Unable to resolve spec");
  libgitErrorCheck(git_signature_default(&tagger, repo),
                   "Unable to create signature");
  libgitErrorCheck(git_tag_create(&oid, repo, tag_name.c_str(),
        target, tagger, message.c_str(), 0), "Unable to create tag");

  git_object_free(target);
  git_signature_free(tagger);
  return *this;
}*/

// git tag <tag_name> <commit_id>
GitHarness& GitHarness::addTagLightweight(std::string tag_name,
                                          git_oid oid) {
  assert(repo != nullptr);
  assert(tag_name.length() > 0);

  // git_oid oid;
  git_object *target;
  char buf[GIT_OID_HEXSZ + 1];
  git_oid_tostr(buf, sizeof(buf), &oid);

  libgitErrorCheck(git_revparse_single(&target, repo, buf),
                   "Unable to identify commit");
  libgitErrorCheck(
      git_tag_create_lightweight(&oid, repo, tag_name.c_str(), target, 0),
      "Unable to create lightweight tag");

  git_object_free(target);
  return *this;
}

git_oid GitHarness::getLatestCommitId() {
  assert(commit_ids.size() > 0);
  return commit_ids.back();
}

git_repository* GitHarness::getGitRepo() {
  return repo;
}

bool directoryExists(const std::string &directory) {
  if (!directory.empty()) {
    if (access(directory.c_str(), 0) == 0) {
      struct stat status;
      stat(directory.c_str(), &status);

      if (status.st_mode & S_IFDIR)
        return true;
    }
  }

  return false;
}

bool fileExists(const std::string &name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

void executeSystemCommand(std::string command, std::string error_msg) {
  int status = system(command.c_str());
  if (status != 0 && error_msg.length() > 0) {
    std::cerr << error_msg << std::endl;
    exit(1);
  }
}

void libgitErrorCheck(int error, const char *msg) {
  if (!error)
    return;
  std::cerr << msg << std::endl;
  exit(1);
}

void convertToCharArray(std::vector<std::string> *from,
                        std::vector<char*> *to) {
  to->reserve(from->size());
  for (auto& s : *from)
    to->push_back(&s[0]);
}

#ifndef NDEBUG
#define NDEBUG 1
#endif

