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

#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "git_harness.hpp"
#include "sentinel/util/filesystem.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace sentinel {

GitHarness::GitHarness(std::string& dir_name) : repo(nullptr) {
  git_libgit2_init();

  // Create the directory.
  // Exit with error message if it already existed or fail to generate
  if (util::filesystem::isDirectory(dir_name)) {
    throw IOException(EEXIST);
  }

  util::filesystem::createDirectory(dir_name);

  // git init
  if (git_repository_init(&repo, dir_name.c_str(), 0) != 0) {
    std::cerr << "git init fails." << std::endl;
    exit(1);
  }

  repo_path = dir_name;
}

// Create a folder within git directory.
// The folder path should be relative w.r.t. repo path
GitHarness& GitHarness::addFolder(const std::string& folder_name) {
  if (util::filesystem::isDirectory(repo_path+"/"+folder_name)) {
    throw IOException(EEXIST);
  }

  util::filesystem::createDirectory(repo_path + "/" + folder_name);

  return *this;
}

// Create a file at specified location (relative path w.r.t repo path)
GitHarness& GitHarness::addFile(const std::string& filename,
                                const std::string& content) {
  if (util::filesystem::isRegularFile(repo_path+"/"+filename)) {
    throw IOException(EEXIST);
  }

  std::string new_filename_str = repo_path + "/" + filename;
  std::ofstream new_file(new_filename_str.c_str(), std::ios::app);
  if (new_file.fail()) {
    throw std::runtime_error("Fail to make file " + filename);
  }

  new_file << content;
  if (new_file.bad()) {
    throw std::runtime_error("Error writing to file " + filename);
  }

  new_file.close();
  return *this;
}

// Add code to file.
// If line number is negative or exceed file length, the code is appended to
// end of file.
// By default, code is appended to end of file.
GitHarness& GitHarness::addCode(const std::string& filename,
                                const std::string& content,
                                int line, int col) {
  std::string filename_full = repo_path + "/" + filename;
  if (!util::filesystem::isRegularFile(filename_full)) {
    throw IOException(EINVAL);
  }

  // Create a temporary file to modify file content.
  // Temporary filename is filename.temp
  std::ifstream orig_file(filename_full, std::ios_base::in);
  std::ofstream changed_file(filename_full+".temp",
                             std::ios_base::out | std::ios_base::trunc);

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
    if (col <= 0 || col > line_str.length()) {
      changed_file << line_str << content << std::endl;
    } else {
      changed_file << line_str.substr(0, col-1) << content
                   << line_str.substr(col-1, std::string::npos) << std::endl;
    }
  }

  // If line number is negative or exceeding length of file, content is added
  // to end of file.
  if (line <= 0 || line > line_idx) {
    changed_file << content;
  }

  orig_file.close();
  changed_file.close();

  // replace original file with changed file.
  util::filesystem::rename(filename_full+".temp", filename_full);
  return *this;
}

// Delete multiple line of codes. Code line starts from 1.
GitHarness& GitHarness::deleteCode(const std::string& filename,
                                   const std::vector<int>& lines) {
  std::string filename_full = repo_path + "/" + filename;
  if (!util::filesystem::isRegularFile(filename_full)) {
    throw IOException(EINVAL);
  }

  // Create a temporary file to modify file content.
  // Temporary filename is filename.temp
  std::ifstream orig_file(filename_full);
  std::ofstream changed_file(filename_full+".temp");

  int line_idx = 0;
  std::string line_str;
  while (std::getline(orig_file, line_str)) {
    line_idx += 1;

    // If line_idx is in vector lines, then the line is skipped.
    // Otherwise, the line is added to temp file.
    if (std::find(lines.begin(), lines.end(), line_idx) != lines.end()) {
      continue;
    }

    changed_file << line_str << std::endl;
  }

  orig_file.close();
  changed_file.close();

  // replace original file with changed file.
  util::filesystem::rename(filename_full+".temp", filename_full);
  return *this;
}

// git add <filenames>
GitHarness& GitHarness::stageFile(std::vector<std::string>& filenames) {
  for (const auto& filename : filenames) {
    if (!util::filesystem::isRegularFile(repo_path + "/" + filename)) {
      throw IOException(EINVAL);
    }
  }

  git_index_matched_path_cb matched_cb = nullptr;
  git_index* index;
  std::vector<char*> argv;
  argv.reserve(filenames.size());
  for (auto& s : filenames) {
    argv.push_back(&s[0]);
  }

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
GitHarness& GitHarness::commit(const std::string& message) {
  git_oid new_commit_id, tree_oid;
  git_tree* tree;
  git_index* index;
  git_object* parent = nullptr;
  git_reference* ref = nullptr;
  git_signature* signature;

  int error = git_revparse_ext(&parent, &ref, repo, "HEAD");
  if (error == GIT_ENOTFOUND) {
    std::cerr << "HEAD not found. Creating first commit\n";
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
                                       tree, (parent != nullptr) ? 1 : 0,
                                       parent),
                   "Error creating commit");
  commit_ids.push_back(new_commit_id);

  git_object_free(parent);
  git_reference_free(ref);
  git_index_free(index);
  git_signature_free(signature);
  git_tree_free(tree);

  return *this;
}

// git tag -a <tag_name> -m "message"
/*GitHarness& GitHarness::addTag(std::string tag_name, std::string commit_id,
                               std::string message) {
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
GitHarness& GitHarness::addTagLightweight(const std::string& tag_name,
                                          const std::string& oid_str) {
  if (tag_name.empty()) {
    throw std::runtime_error("Tag name is empty");
  }

  git_oid oid;
  git_object *target;
  const char* buf = oid_str.c_str();

  libgitErrorCheck(git_oid_fromstrp(&oid, buf),
                   "Error retrieving commit");
  libgitErrorCheck(git_revparse_single(&target, repo,
                   static_cast<const char*>(buf)),
                   "Unable to identify commit");
  libgitErrorCheck(
      git_tag_create_lightweight(&oid, repo, tag_name.c_str(), target, 0),
      "Unable to create lightweight tag");

  git_object_free(target);

  return *this;
}

// git tag <tag_name>
GitHarness& GitHarness::addTagLightweight(const std::string& tag_name) {
  std::string oid = getLatestCommitId();
  addTagLightweight(tag_name, oid);
  return *this;
}

std::string GitHarness::getLatestCommitId() {
  if (commit_ids.empty()) {
    throw std::range_error("No commit was made yet");
  }

  char buf[GIT_OID_HEXSZ + 1];
  git_oid_tostr(static_cast<char*>(buf), sizeof(buf), &commit_ids.back());
  std::string ret(static_cast<char*>(buf), GIT_OID_HEXSZ + 1);
  return ret;
}

git_repository* GitHarness::getGitRepo() {
  return repo;
}

void GitHarness::libgitErrorCheck(int error, const char* msg) {
  if (error == 0) {
    return;
  }

  std::cerr << msg << std::endl;
  exit(1);
}

}  // namespace sentinel


