/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fnmatch.h>
#include <fmt/core.h>
#include <git2.h>
#include <iostream>
#include <algorithm>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "sentinel/GitRepository.hpp"
#include "sentinel/GitSourceTree.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/util/string.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"

namespace fs = std::experimental::filesystem;

namespace sentinel {

const char* cGitRepositoryLoggerName = "GitRepository";

/**
 * @brief git_xxx pointer free guard
 */
template <typename T, void Free(T*)>
class SafeGit2ObjPtr {
 private:
  T* obj;

 public:
  SafeGit2ObjPtr() : obj(nullptr) {}
  ~SafeGit2ObjPtr() {
    if (obj) {
      Free(obj);
    }
  }

  /**
   * @brief get T pointer type member address to assign value
   */
  T** getPtr() {
    return &obj;
  }

  /**
   * @brief cast to T pointer
   */
  explicit operator T*() {
    return obj;
  }

  /**
   * @brief cast to C
   */
  template <typename C>
  C cast() {
    return reinterpret_cast<C>(obj);  // NOLINT
  }

  /**
   * @brief check member obj is nullptr
   */
  bool isNull() {
    return obj == nullptr;
  }
};

/**
 * @brief Diff iteration callback payload
 */
class DiffData {
 public:
  /**
   * @brief Default constructor
   *
   * @param gitRepo git repository
   * @param gitWorkdir actual git working directory root (may differ from sourceRoot)
   */
  DiffData(GitRepository* gitRepo, std::string gitWorkdir) : mGitRepo(gitRepo), mGitWorkdir(std::move(gitWorkdir)) {}

  /**
   * @brief add Source Line
   *
   * @param file source file name (relative to git workdir)
   * @param line line number
   */
  void addSourceLine(const std::string& file, int line) {
    fs::path filePath(mGitWorkdir);
    filePath.append(file);
    if (mGitRepo->isTargetPath(filePath)) {
      mSourceLines.push_back(SourceLine(filePath, line));
    }
  }

  /**
   * @brief get source lines
   */
  SourceLines getSourceLines() {
    return mSourceLines;
  }

 private:
  /// diff result
  SourceLines mSourceLines;
  /// git repository instance
  GitRepository* mGitRepo;
  /// actual git working directory root
  std::string mGitWorkdir;
};

static void getDiffFromTree(git_repository* repo, git_tree* tree, git_diff_options* opts, DiffData& d) {  // NOLINT
  SafeGit2ObjPtr<git_diff, git_diff_free> diff;

  if (git_diff_tree_to_workdir_with_index(diff.getPtr(), static_cast<git_repository*>(repo),
                                          static_cast<git_tree*>(tree), opts) != 0) {
    throw RepositoryException("Failed to find diff");
  }

  if (git_diff_foreach(
          static_cast<git_diff*>(diff),
          [](const git_diff_delta*, float, void*) {
            // each_file_cb
            return 0;
          },
          [](const git_diff_delta*, const git_diff_binary*, void*) {
            // each_binary_cb
            return 0;
          },
          [](const git_diff_delta*, const git_diff_hunk*, void*) {
            // each_hunk_cb
            return 0;
          },
          [](const git_diff_delta* delta, const git_diff_hunk* hunk, const git_diff_line* line, void* payload) {
            // each_line_cb
            auto d = reinterpret_cast<DiffData*>(payload);  // NOLINT
            auto logger = Logger::getLogger(cGitRepositoryLoggerName);
            logger->debug(fmt::format("{}:{}:{}", line->origin, delta->new_file.path, line->new_lineno));

            if (line->origin == '+') {
              d->addSourceLine(delta->new_file.path, line->new_lineno);
            }
            return 0;
          },
          &d) != 0) {
    throw RepositoryException("Failed to diff iterate");
  }
}

/**
 * @brief Recursively collect all git repo roots at or below dir.
 *
 * A repo root is any directory that contains a ".git" entry.  Recursion
 * continues into subdirectories so that arbitrarily-nested multi-repo
 * layouts (e.g. Android repo) are discovered fully.
 */
static std::vector<std::experimental::filesystem::path> collectGitRepos(const std::experimental::filesystem::path& dir) {
  namespace fs = std::experimental::filesystem;
  std::vector<fs::path> result;
  if (!fs::is_directory(dir)) {
    return result;
  }

  if (fs::exists(dir / ".git")) {
    result.push_back(dir);
  }

  try {
    for (const auto& entry : fs::directory_iterator(dir)) {
      if (!fs::is_directory(entry.path())) {
        continue;
      }
      if (entry.path().filename() == ".git") {
        continue;  // never recurse into .git itself
      }
      auto sub = collectGitRepos(entry.path());
      result.insert(result.end(), sub.begin(), sub.end());
    }
  } catch (const fs::filesystem_error&) {
    // Skip directories we cannot read.
  }
  return result;
}

/**
 * @brief Apply diff scope logic ("commit" or "all") to a single open repo.
 *
 * Extracted from getSourceLines() so it can be called for each repo in a
 * multi-repo walk without duplicating the commit-traversal logic.
 */
static void applyDiffScope(git_repository* repo, const std::string& scope, git_diff_options* opts, DiffData& d,
                            const std::string& gitWorkdir) {
  auto logger = Logger::getLogger(cGitRepositoryLoggerName);

  if (scope == "all") {
    getDiffFromTree(repo, nullptr, opts, d);
    return;
  }

  // scope == "commit"
  SafeGit2ObjPtr<git_object, git_object_free> obj;
  SafeGit2ObjPtr<git_tree, git_tree_free> tree;
  SafeGit2ObjPtr<git_reference, git_reference_free> ref;

  if (git_reference_lookup(ref.getPtr(), repo, "refs/tags/devtool-base") == 0) {
    logger->info(fmt::format("found devtool tag in {}", gitWorkdir));
    int error_code = git_reference_peel(obj.getPtr(), static_cast<git_reference*>(ref), GIT_OBJ_COMMIT);
    if (error_code != 0) {
      throw RepositoryException(fmt::format("Failed to peel tag: error code {}", error_code));
    }
  } else {
    git_revparse_single(obj.getPtr(), repo, "HEAD^{commit}");
  }

  if (!obj.isNull()) {
    SafeGit2ObjPtr<git_commit, git_commit_free> parent_commit;
    if (git_commit_parentcount(obj.cast<const git_commit*>()) > 0) {
      if (git_commit_parent(parent_commit.getPtr(), obj.cast<const git_commit*>(), 0) != 0) {
        throw RepositoryException("Failed to find parent commit");
      }
      if (git_commit_tree(tree.getPtr(), static_cast<git_commit*>(parent_commit)) != 0) {
        throw RepositoryException("Failed to find parent tree");
      }
    }
  } else {
    logger->warn(fmt::format("No HEAD commit or devtool tag found in {}", gitWorkdir));
  }

  // tree may be null here (no parent commit) → diff from empty tree = all files.
  getDiffFromTree(repo, static_cast<git_tree*>(tree), opts, d);
}

GitRepository::GitRepository(const std::string& path, const std::vector<std::string>& extensions,
                             const std::vector<std::string>& patterns, const std::vector<std::string>& excludes) {
  git_libgit2_init();

  auto logger = Logger::getLogger(cGitRepositoryLoggerName);

  try {
    mSourceRoot = fs::canonical(path);
  } catch (const fs::filesystem_error& e) {
    throw InvalidArgumentException(fmt::format("source_root option error: {}", e.what()));
  }
  logger->info(fmt::format("source root: {}", mSourceRoot.string()));

  if (!extensions.empty()) {
    std::transform(extensions.begin(), extensions.end(), std::back_inserter(mExtensions),
                   [](auto extension) -> std::string {
                     return extension.at(0) == '.' ? extension : fmt::format(".{}", extension);
                   });
  }

  logger->debug(fmt::format("patterns: {}", string::join(", ", patterns)));
  mPatterns = patterns;

  logger->debug(fmt::format("excludes: {}", string::join(", ", excludes)));
  mExcludes = excludes;
}

GitRepository::~GitRepository() {
  git_libgit2_shutdown();
}

bool GitRepository::isTargetPath(const std::experimental::filesystem::path& path, bool checkExtension) {
  auto logger = Logger::getLogger(cGitRepositoryLoggerName);
  fs::path canonicalPath;

  // Canonicalize the path (relative paths are resolved relative to sourceRoot).
  try {
    canonicalPath = fs::canonical(path, getSourceRoot());
  } catch (const fs::filesystem_error&) {
    if (path.is_absolute()) {
      canonicalPath = path;
    } else {
      return false;
    }
  }

  // Reject files that are not under sourceRoot.  This matters in multi-repo
  // setups where the enclosing repo's diff may include files from sibling
  // directories outside the user-specified sourceRoot.
  {
    auto mm = std::mismatch(mSourceRoot.begin(), mSourceRoot.end(), canonicalPath.begin(), canonicalPath.end());
    if (mm.first != mSourceRoot.end()) {
      return false;
    }
  }

  auto matcher = [&](const auto& exclude) {
    return fnmatch(exclude.c_str(), canonicalPath.string().c_str(), 0) == 0;
  };
  if (std::any_of(mExcludes.begin(), mExcludes.end(), matcher)) {
    return false;
  }

  if (checkExtension) {
    std::string extension = path.extension();

    if (!this->mExtensions.empty() &&
        std::find(this->mExtensions.begin(), this->mExtensions.end(), extension) == this->mExtensions.end()) {
      return false;
    }
  }

  return true;
}

SourceLines GitRepository::getSourceLines(const std::string& scope) {
  namespace fs = std::experimental::filesystem;
  auto logger = Logger::getLogger(cGitRepositoryLoggerName);

  if (scope != "commit" && scope != "all") {
    throw InvalidArgumentException(fmt::format("scope '{}' is invalid.", scope));
  }

  // Step 1: find all git repos directly at or below sourceRoot (multi-repo support).
  // In Android-style workspaces there is no top-level .git; each component has its own.
  std::vector<fs::path> repoPaths = collectGitRepos(getSourceRoot());

  // Step 2: also probe upward for an enclosing repo.  This handles the case where
  // sourceRoot is a subdirectory inside a single git repo without nested repos.
  // The enclosing repo's workdir is recorded for deduplication below.
  std::string enclosingWorkdir;
  {
    SafeGit2ObjPtr<git_repository, git_repository_free> probe;
    if (git_repository_open_ext(probe.getPtr(), getSourceRoot().c_str(), 0, nullptr) == 0) {
      const char* rawWorkdir = git_repository_workdir(static_cast<git_repository*>(probe));
      if (rawWorkdir) {
        enclosingWorkdir = fs::canonical(fs::path(rawWorkdir)).string();
      }
    }
  }

  if (repoPaths.empty() && enclosingWorkdir.empty()) {
    throw RepositoryException("Fail to open repository");
  }

  // Track which git workdirs have already been processed to avoid double-counting
  // (e.g. when sourceRoot itself has .git it appears in both repoPaths and enclosingWorkdir).
  std::set<std::string> processedWorkdirs;
  SourceLines allSourceLines;

  // Helper: build git_diff_options from mPatterns.
  // cstrs must outlive opts; both are declared together in each call site below.
  auto buildOpts = [&](std::vector<char*>& cstrs) -> git_diff_options {
    cstrs.resize(mPatterns.size());
    std::transform(mPatterns.begin(), mPatterns.end(), cstrs.begin(),
                   [](auto& s) { return const_cast<char*>(s.c_str()); });
    git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
    git_strarray pathspec = {cstrs.data(), cstrs.size()};
    opts.pathspec = pathspec;
    return opts;
  };

  // Step 3: process each nested/direct repo found under sourceRoot.
  // Use GIT_REPOSITORY_OPEN_NO_SEARCH so we open exactly the repo at repoPath.
  for (const auto& repoPath : repoPaths) {
    SafeGit2ObjPtr<git_repository, git_repository_free> repo;
    if (git_repository_open_ext(repo.getPtr(), repoPath.c_str(), GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr) != 0) {
      logger->warn(fmt::format("Failed to open git repo at {}", repoPath.string()));
      continue;
    }

    const char* rawWorkdir = git_repository_workdir(static_cast<git_repository*>(repo));
    if (!rawWorkdir) {
      logger->warn(fmt::format("Repo at {} has no working directory, skipping", repoPath.string()));
      continue;
    }
    std::string gitWorkdir = fs::canonical(fs::path(rawWorkdir)).string();
    logger->info(fmt::format("git workdir: {}", gitWorkdir));
    processedWorkdirs.insert(gitWorkdir);

    std::vector<char*> cstrs;
    git_diff_options opts = buildOpts(cstrs);
    DiffData d(this, gitWorkdir);
    try {
      applyDiffScope(static_cast<git_repository*>(repo), scope, &opts, d, gitWorkdir);
    } catch (const RepositoryException& e) {
      logger->warn(fmt::format("diff failed for {}: {}", gitWorkdir, e.what()));
      continue;
    }

    SourceLines lines = d.getSourceLines();
    allSourceLines.insert(allSourceLines.end(), lines.begin(), lines.end());
  }

  // Step 4: process the enclosing repo if its workdir hasn't been handled yet.
  // This covers the case where sourceRoot is a subdir of a git repo (no nested .git found).
  // isTargetPath() filters the diff to only files under sourceRoot, so sibling
  // directories of sourceRoot inside the enclosing repo are excluded automatically.
  if (!enclosingWorkdir.empty() && processedWorkdirs.find(enclosingWorkdir) == processedWorkdirs.end()) {
    SafeGit2ObjPtr<git_repository, git_repository_free> repo;
    if (git_repository_open_ext(repo.getPtr(), getSourceRoot().c_str(), 0, nullptr) == 0) {
      const char* rawWorkdir = git_repository_workdir(static_cast<git_repository*>(repo));
      if (rawWorkdir) {
        std::string gitWorkdir = fs::canonical(fs::path(rawWorkdir)).string();
        logger->info(fmt::format("git workdir (enclosing): {}", gitWorkdir));

        std::vector<char*> cstrs;
        git_diff_options opts = buildOpts(cstrs);
        DiffData d(this, gitWorkdir);
        try {
          applyDiffScope(static_cast<git_repository*>(repo), scope, &opts, d, gitWorkdir);
        } catch (const RepositoryException& e) {
          if (repoPaths.empty()) {
            throw;  // Only repo available; propagate so the caller sees the error.
          }
          logger->warn(fmt::format("diff failed for enclosing repo {}: {}", gitWorkdir, e.what()));
        }

        SourceLines lines = d.getSourceLines();
        allSourceLines.insert(allSourceLines.end(), lines.begin(), lines.end());
      }
    }
  }

  return allSourceLines;
}

std::shared_ptr<SourceTree> GitRepository::getSourceTree() {
  return std::make_shared<GitSourceTree>(getSourceRoot());
}

}  // namespace sentinel
