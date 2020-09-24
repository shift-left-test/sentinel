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
#include "git-harness/GitHarness.hpp"
#include "sentinel/util/os.hpp"
#include "sentinel/exceptions/IOException.hpp"

namespace sentinel {

GitHarness::GitHarness(const std::string& dir_name) :
    repo(nullptr) {
  git_libgit2_init();

  // Create the directory.
  // Exit with error message if it already existed or fail to generate
  if (os::path::isDirectory(dir_name)) {
    throw IOException(EEXIST);
  }

  os::createDirectory(dir_name);

  // git init
  if (git_repository_init(&repo, dir_name.c_str(), 0) != 0) {
    std::cerr << "git init fails." << std::endl;
    exit(1);
  }

  repo_path = dir_name;
}

GitHarness::~GitHarness() {
  git_repository_free(repo);
  git_libgit2_shutdown();
}

// Create a folder within git directory.
// The folder path should be relative w.r.t. repo path
GitHarness& GitHarness::addFolder(const std::string& folder_name) {
  if (os::path::isDirectory(repo_path+"/"+folder_name)) {
    throw IOException(EEXIST);
  }

  os::createDirectory(repo_path + "/" + folder_name);

  return *this;
}

// Create a file at specified location (relative path w.r.t repo path)
GitHarness& GitHarness::addFile(const std::string& filename,
                                const std::string& content) {
  if (os::path::isRegularFile(repo_path+"/"+filename)) {
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
                                std::size_t line, std::size_t col) {
  std::string filename_full = repo_path + "/" + filename;
  if (!os::path::isRegularFile(filename_full)) {
    throw IOException(EINVAL);
  }

  // Create a temporary file to modify file content.
  // Temporary filename is filename.temp
  std::ifstream orig_file(filename_full, std::ios_base::in);
  std::ofstream changed_file(filename_full+".temp",
                             std::ios_base::out | std::ios_base::trunc);

  std::size_t line_idx = 0;
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
    if (col == 0 || col > line_str.length()) {
      changed_file << line_str << content << std::endl;
    } else {
      changed_file << line_str.substr(0, col-1) << content
                   << line_str.substr(col-1, std::string::npos) << std::endl;
    }
  }

  // If line number is negative or exceeding length of file, content is added
  // to end of file.
  if (line == 0 || line > line_idx) {
    changed_file << content;
  }

  orig_file.close();
  changed_file.close();

  // replace original file with changed file.
  os::rename(filename_full+".temp", filename_full);
  return *this;
}

// Delete multiple line of codes. Code line starts from 1.
GitHarness& GitHarness::deleteCode(const std::string& filename,
                                   const std::vector<std::size_t>& lines) {
  std::string filename_full = repo_path + "/" + filename;
  if (!os::path::isRegularFile(filename_full)) {
    throw IOException(EINVAL);
  }

  // Create a temporary file to modify file content.
  // Temporary filename is filename.temp
  std::ifstream orig_file(filename_full);
  std::ofstream changed_file(filename_full+".temp");

  std::size_t line_idx = 0;
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
  os::rename(filename_full+".temp", filename_full);
  return *this;
}

// git add <filenames>
GitHarness& GitHarness::stageFile(std::vector<std::string> filenames) {
  auto noRegularFile = [&](const auto& s) {
    auto path = os::path::join(repo_path, s);
    return !os::path::isRegularFile(path);
  };
  if (std::any_of(filenames.begin(), filenames.end(), noRegularFile)) {
    throw IOException(EINVAL);
  }

  git_index_matched_path_cb matched_cb = nullptr;
  git_index* index;
  std::vector<char*> argv;
  std::transform(filenames.begin(), filenames.end(), std::back_inserter(argv),
                 [](std::string& s) -> char* {return &s[0];});

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

GitHarness& GitHarness::createBranch(const std::string& branch_name) {
  return createBranch(branch_name, "HEAD");
}

GitHarness& GitHarness::createBranch(const std::string& branch_name,
                                     const std::string& commit_id) {
  git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
  git_reference* branch = nullptr;
  git_commit* target_commit = nullptr;
  git_oid oid;
  const char* buf = commit_id.c_str();
  git_object* object = nullptr;

  checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
  if (git_oid_fromstrp(&oid, buf) < 0) {
    libgitErrorCheck(git_reference_name_to_id(&oid, repo, commit_id.c_str()),
                     "Fail to retrieve git_oid");
  }

  libgitErrorCheck(git_commit_lookup(&target_commit, repo, &oid),
                    "Fail to retrieve commit");
  libgitErrorCheck(git_object_lookup(&object, repo, &oid, GIT_OBJ_ANY),
                   "Fail to retrieve object");
  libgitErrorCheck(git_checkout_tree(
                       repo,
                       object,
                       &checkout_opts),
                   "Fail to checkout tree");
  libgitErrorCheck(git_branch_create(&branch, repo, branch_name.c_str(),
                                     target_commit, 0),
                   "Fail to create new branch");

  const char* target_head = git_reference_name(branch);
  libgitErrorCheck(git_repository_set_head(repo, target_head),
                   "Fail to update HEAD reference");

  git_object_free(object);
  git_reference_free(branch);
  git_commit_free(target_commit);

  return *this;
}

GitHarness& GitHarness::checkoutBranch(const std::string& branch_name) {
  git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
  git_oid oid;
  git_object* object = nullptr;
  git_reference* branch = nullptr;

  checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
  libgitErrorCheck(git_branch_lookup(&branch, repo, branch_name.c_str(),
                                     GIT_BRANCH_LOCAL),
                   "Fail to retrieve branch");
  libgitErrorCheck(git_object_lookup(&object, repo,
                                     git_reference_target(branch),
                                     GIT_OBJ_ANY),
                   "Fail to retrieve object");
  libgitErrorCheck(git_checkout_tree(repo, object, &checkout_opts),
                   "Fail to checkout tree");

  const char* target_head = git_reference_name(branch);
  libgitErrorCheck(git_repository_set_head(repo, target_head),
                   "Fail to update HEAD reference");

  git_object_free(object);
  git_reference_free(branch);

  return *this;
}

GitHarness& GitHarness::merge(const std::vector<std::string>& branches) {
  for (const std::string& branch : branches) {
    merge(branch);
  }

  return *this;
}


GitHarness& GitHarness::merge(const std::string& branch) {
  // simple merge for non-conflict branches
  git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
  git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

  merge_opts.flags = GIT_MERGE_FAIL_ON_CONFLICT;
  merge_opts.file_flags = GIT_MERGE_FILE_STYLE_DIFF3;

  checkout_opts.checkout_strategy = GIT_CHECKOUT_FORCE |
                                    GIT_CHECKOUT_ALLOW_CONFLICTS;

  git_annotated_commit* target = nullptr;
  git_reference* branch_ref = nullptr;

  libgitErrorCheck(git_branch_lookup(&branch_ref, repo, branch.c_str(),
                                     GIT_BRANCH_LOCAL),
                   "Fail to retrieve branch");
  libgitErrorCheck(git_annotated_commit_from_ref(&target, repo,
                                                 branch_ref),
                   "Fail to create git_annotated_commit");
  libgitErrorCheck(
      git_merge(
          repo,
          const_cast<const git_annotated_commit**>(&target),  // NOLINT
          1, &merge_opts, &checkout_opts),
      "merge failed");

  // Prepare commit message
  std::string commit_msg{"Merge branch " + branch + " into "};
  git_reference* head_ref;
  libgitErrorCheck(git_repository_head(&head_ref, repo),
                   "failed to get repo HEAD");
  commit_msg += git_reference_name(head_ref);

  // Prepare parent commits
  git_commit* parents[2];

  libgitErrorCheck(git_commit_lookup(&parents[0], repo,
                                     git_reference_target(head_ref)),
                   "Fail to find HEAD commit");
  libgitErrorCheck(git_commit_lookup(&parents[1], repo,
                                     git_annotated_commit_id(target)),
                   "Fail to find target branch commit");

  // Create commit signature
  git_signature* signature;
  libgitErrorCheck(git_signature_now(&signature, "Loc Phan",
                                     "loc.phan@lge.com"),
                   "Error creating signature");

  // Prepare commit tree and commit
  git_oid commit_oid, tree_oid;
  git_tree* tree;
  git_index* index;

  libgitErrorCheck(git_repository_index(&index, repo),
                   "Could not open repository index");
  libgitErrorCheck(git_index_write_tree(&tree_oid, index),
                   "Could not write tree");
  libgitErrorCheck(git_tree_lookup(&tree, repo, &tree_oid),
                   "Error looking up tree");
  libgitErrorCheck(git_commit_create(
                       &commit_oid, repo, git_reference_name(head_ref),
                       signature, signature, "UTF-8", commit_msg.c_str(),
                       tree, 2,
                       const_cast<const git_commit **>(parents)),  // NOLINT
                   "Fail to create merge commit");
  commit_ids.push_back(commit_oid);

  // Clean up
  git_repository_state_cleanup(repo);
  git_reference_free(branch_ref);
  git_annotated_commit_free(target);
  git_commit_free(parents[0]);
  git_commit_free(parents[1]);
  git_reference_free(head_ref);
  git_signature_free(signature);
  git_tree_free(tree);
  git_index_free(index);

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
  std::cerr << error << std::endl;
  throw IOException(EINVAL, msg);
  // std::cerr << msg << std::endl;
  exit(1);
}

}  // namespace sentinel
