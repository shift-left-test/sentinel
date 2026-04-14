/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <fnmatch.h>
#include <git2.h>
#include <algorithm>
#include <filesystem>  // NOLINT
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "sentinel/GitRepository.hpp"
#include "sentinel/GitSourceTree.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"

namespace sentinel {

namespace fs = std::filesystem;

/**
 * @brief git_xxx pointer free guard
 */
template <typename T, void Free(T*)>
class SafeGit2ObjPtr {
 private:
  T* mObj;

 public:
  SafeGit2ObjPtr() : mObj(nullptr) {
  }
  ~SafeGit2ObjPtr() {
    if (mObj) {
      Free(mObj);
    }
  }

  /**
   * @brief get T pointer type member address to assign value
   */
  T** getPtr() {
    return &mObj;
  }

  /**
   * @brief cast to T pointer
   */
  explicit operator T*() {
    return mObj;
  }

  /**
   * @brief cast to C
   */
  template <typename C>
  C cast() const {
    return reinterpret_cast<C>(mObj);
  }

  /**
   * @brief check member obj is nullptr
   */
  bool isNull() const {
    return mObj == nullptr;
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
  DiffData(GitRepository* gitRepo, std::filesystem::path gitWorkdir) :
      mGitRepo(gitRepo), mGitWorkdir(std::move(gitWorkdir)) {
  }

  /**
   * @brief add Source Line
   *
   * @param file source file name (relative to git workdir)
   * @param line line number
   */
  void addSourceLine(const std::string& file, int line) {
    fs::path filePath = mGitWorkdir / file;
    if (mGitRepo->isTargetPath(filePath)) {
      mSourceLines.push_back(SourceLine(filePath, line));
    }
  }

  /**
   * @brief get source lines
   */
  SourceLines getSourceLines() const {
    return mSourceLines;
  }

 private:
  /// diff result
  SourceLines mSourceLines;
  /// git repository instance
  GitRepository* mGitRepo;
  /// actual git working directory root
  fs::path mGitWorkdir;
};

/**
 * @brief Iterate a git_diff and collect added lines into DiffData.
 */
static void collectAddedLines(git_diff* diff, DiffData* d) {
  if (git_diff_foreach(
          diff,
          [](const git_diff_delta*, float, void*) { return 0; },
          [](const git_diff_delta*, const git_diff_binary*, void*) { return 0; },
          [](const git_diff_delta*, const git_diff_hunk*, void*) { return 0; },
          [](const git_diff_delta* delta, const git_diff_hunk*,
             const git_diff_line* line, void* payload) {
            if (line->origin == '+') {
              reinterpret_cast<DiffData*>(payload)->addSourceLine(
                  delta->new_file.path, line->new_lineno);
            }
            return 0;
          },
          d) != 0) {
    throw RepositoryException("Failed to diff iterate");
  }
}

static void applyDiffAll(git_repository* repo,
                         git_diff_options* opts, DiffData* d) {
  SafeGit2ObjPtr<git_diff, git_diff_free> diff;
  if (git_diff_tree_to_workdir_with_index(
          diff.getPtr(), repo, nullptr, opts) != 0) {
    throw RepositoryException("Failed to find diff");
  }
  collectAddedLines(static_cast<git_diff*>(diff), d);
}

/**
 * @brief Recursively collect all git repo roots at or below dir.
 *
 * A repo root is any directory that contains a ".git" entry.  Recursion
 * continues into subdirectories so that arbitrarily-nested multi-repo
 * layouts (e.g. Android repo) are discovered fully.
 */
static std::vector<fs::path> collectGitRepos(const std::filesystem::path& dir,
                                             const std::vector<std::filesystem::path>& skipDirs) {
  std::vector<fs::path> result;
  if (!fs::is_directory(dir)) {
    return result;
  }

  if (fs::exists(dir / ".git")) {
    result.push_back(dir);
  }

  try {
    std::vector<fs::directory_entry> entries;
    std::copy(fs::directory_iterator(dir), fs::directory_iterator(), std::back_inserter(entries));
    std::sort(entries.begin(), entries.end(),
              [](const fs::directory_entry& lhs, const fs::directory_entry& rhs) {
                return lhs.path().string() < rhs.path().string();
              });
    for (const auto& entry : entries) {
      // Skip symlinks: following them risks infinite loops (self-referential
      // links) and traversal outside the source root.
      if (fs::is_symlink(entry.path())) {
        continue;
      }
      if (!fs::is_directory(entry.path())) {
        continue;
      }
      // Skip hidden directories (e.g. .git, .cache, .ccache, .github).
      // They never contain nested source repos and traversing them is wasteful.
      auto filename = entry.path().filename().string();
      if (!filename.empty() && filename.front() == '.') {
        continue;
      }
      // Skip sentinel-managed directories (workspace, output, etc.)
      if (std::any_of(skipDirs.begin(), skipDirs.end(),
                       [&](const auto& sd) {
                         return entry.path() == sd;
                       })) {
        continue;
      }
      auto sub = collectGitRepos(entry.path(), skipDirs);
      result.insert(result.end(), std::make_move_iterator(sub.begin()), std::make_move_iterator(sub.end()));
    }
  } catch (const fs::filesystem_error& e) {
    Logger::warn("Skipping unreadable directory '{}': {}", dir, e.what());
  }
  return result;
}

/**
 * @brief Diff between two commit trees (for --from: merge-base to HEAD).
 */
static void applyDiffTreeToTree(git_repository* repo,
                                const std::string& fromRef,
                                git_diff_options* opts, DiffData* d,
                                const std::filesystem::path& gitWorkdir) {
  // Resolve HEAD first (needed for root-commit detection)
  SafeGit2ObjPtr<git_object, git_object_free> headObj;
  if (git_revparse_single(headObj.getPtr(), repo, "HEAD") != 0) {
    throw RepositoryException(
        fmt::format("Failed to resolve HEAD in {}", gitWorkdir));
  }

  SafeGit2ObjPtr<git_object, git_object_free> headPeeled;
  if (git_object_peel(headPeeled.getPtr(),
                      static_cast<git_object*>(headObj),
                      GIT_OBJECT_COMMIT) != 0) {
    throw RepositoryException(
        fmt::format("HEAD does not point to a commit in {}", gitWorkdir));
  }

  // Resolve fromRef — may fail when HEAD is a root commit and fromRef
  // refers to a non-existent parent (e.g. HEAD^, HEAD~1).  In that case
  // we fall back to diffing against an empty tree.
  SafeGit2ObjPtr<git_object, git_object_free> fromObj;
  bool fromResolved =
      git_revparse_single(fromObj.getPtr(), repo, fromRef.c_str()) == 0;

  if (!fromResolved) {
    if (git_commit_parentcount(headPeeled.cast<git_commit*>()) != 0) {
      throw RepositoryException(
          fmt::format("Failed to resolve revision '{}' in {}",
                      fromRef, gitWorkdir));
    }
    SafeGit2ObjPtr<git_tree, git_tree_free> headTree;
    if (git_commit_tree(headTree.getPtr(),
                        headPeeled.cast<git_commit*>()) != 0) {
      throw RepositoryException(
          fmt::format("Failed to get tree for HEAD in {}", gitWorkdir));
    }
    SafeGit2ObjPtr<git_diff, git_diff_free> diff;
    if (git_diff_tree_to_tree(
            diff.getPtr(), repo, nullptr,
            static_cast<git_tree*>(headTree), opts) != 0) {
      throw RepositoryException(
          fmt::format("Failed to diff trees for '{}' in {}",
                      fromRef, gitWorkdir));
    }
    collectAddedLines(static_cast<git_diff*>(diff), d);
    return;
  }

  SafeGit2ObjPtr<git_object, git_object_free> fromPeeled;
  if (git_object_peel(fromPeeled.getPtr(),
                      static_cast<git_object*>(fromObj),
                      GIT_OBJECT_COMMIT) != 0) {
    throw RepositoryException(
        fmt::format("Revision '{}' does not point to a commit in {}",
                    fromRef, gitWorkdir));
  }

  // Compute merge-base
  git_oid baseOid;
  const git_oid* fromOid =
      git_commit_id(fromPeeled.cast<git_commit*>());
  const git_oid* headOid =
      git_commit_id(headPeeled.cast<git_commit*>());
  if (git_merge_base(&baseOid, repo, headOid, fromOid) != 0) {
    throw RepositoryException(
        fmt::format("No common ancestor between HEAD and '{}' in {}",
                    fromRef, gitWorkdir));
  }

  // Get base tree
  SafeGit2ObjPtr<git_commit, git_commit_free> baseCommit;
  if (git_commit_lookup(baseCommit.getPtr(), repo, &baseOid) != 0) {
    throw RepositoryException(
        fmt::format("Failed to lookup merge-base commit in {}",
                    gitWorkdir));
  }

  SafeGit2ObjPtr<git_tree, git_tree_free> baseTree;
  if (git_commit_tree(baseTree.getPtr(),
                      static_cast<git_commit*>(baseCommit)) != 0) {
    throw RepositoryException(
        fmt::format("Failed to get tree for merge-base in {}",
                    gitWorkdir));
  }

  // Get HEAD tree
  SafeGit2ObjPtr<git_tree, git_tree_free> headTree;
  if (git_commit_tree(headTree.getPtr(),
                      headPeeled.cast<git_commit*>()) != 0) {
    throw RepositoryException(
        fmt::format("Failed to get tree for HEAD in {}", gitWorkdir));
  }

  // Diff base tree to HEAD tree
  SafeGit2ObjPtr<git_diff, git_diff_free> diff;
  if (git_diff_tree_to_tree(
          diff.getPtr(), repo,
          static_cast<git_tree*>(baseTree),
          static_cast<git_tree*>(headTree), opts) != 0) {
    throw RepositoryException(
        fmt::format("Failed to diff trees for '{}' in {}",
                    fromRef, gitWorkdir));
  }

  collectAddedLines(static_cast<git_diff*>(diff), d);
}

/**
 * @brief Diff uncommitted changes: staged (tree-to-index) +
 *        unstaged (index-to-workdir).
 *
 * Includes untracked files but excludes ignored files.
 */
static void applyDiffUncommitted(git_repository* repo,
                                 git_diff_options* opts, DiffData* d) {
  // Get HEAD tree (may be null for repos with no commits)
  SafeGit2ObjPtr<git_tree, git_tree_free> headTree;
  {
    SafeGit2ObjPtr<git_object, git_object_free> headObj;
    if (git_revparse_single(headObj.getPtr(), repo, "HEAD") == 0) {
      SafeGit2ObjPtr<git_object, git_object_free> peeled;
      if (git_object_peel(peeled.getPtr(),
                          static_cast<git_object*>(headObj),
                          GIT_OBJECT_COMMIT) == 0) {
        git_commit_tree(headTree.getPtr(),
                        peeled.cast<git_commit*>());
      }
    }
  }

  // 1. diff_tree_to_index (staged changes)
  {
    SafeGit2ObjPtr<git_diff, git_diff_free> diff;
    if (git_diff_tree_to_index(
            diff.getPtr(), repo,
            static_cast<git_tree*>(headTree),
            nullptr, opts) != 0) {
      throw RepositoryException("Failed to diff tree to index");
    }
    collectAddedLines(static_cast<git_diff*>(diff), d);
  }

  // 2. diff_index_to_workdir (unstaged + untracked changes)
  {
    git_diff_options workdirOpts = *opts;
    workdirOpts.flags |= GIT_DIFF_INCLUDE_UNTRACKED | GIT_DIFF_SHOW_UNTRACKED_CONTENT;
    SafeGit2ObjPtr<git_diff, git_diff_free> diff;
    if (git_diff_index_to_workdir(
            diff.getPtr(), repo, nullptr, &workdirOpts) != 0) {
      throw RepositoryException("Failed to diff index to workdir");
    }
    collectAddedLines(static_cast<git_diff*>(diff), d);
  }
}

/**
 * @brief Apply the appropriate diff strategy for a single open repo.
 */
static void applyDiff(git_repository* repo,
                      const std::optional<std::string>& from, bool uncommitted,
                      git_diff_options* opts, DiffData* d,
                      const std::filesystem::path& gitWorkdir) {
  if (!from && !uncommitted) {
    applyDiffAll(repo, opts, d);
  } else {
    if (from) {
      applyDiffTreeToTree(repo, *from, opts, d, gitWorkdir);
    }
    if (uncommitted) {
      applyDiffUncommitted(repo, opts, d);
    }
  }
}

GitRepository::GitRepository(const std::filesystem::path& path, const std::vector<std::string>& extensions,
                             const std::vector<std::string>& patterns) {
  git_libgit2_init();

  mSourceRoot = fs::canonical(path);
  if (!extensions.empty()) {
    std::transform(extensions.begin(), extensions.end(), std::back_inserter(mExtensions),
                   [](auto extension) -> std::string {
                     return extension.at(0) == '.' ? extension : fmt::format(".{}", extension);
                   });
  }

  for (const auto& pat : patterns) {
    if (!pat.empty() && pat.front() == '!') {
      mExcludes.push_back(pat.substr(1));
    } else {
      mPatterns.push_back(pat);
    }
  }
}

GitRepository::~GitRepository() {
  git_libgit2_shutdown();
}

const std::filesystem::path& GitRepository::getSourceRoot() const {
  return mSourceRoot;
}

void GitRepository::addSkipDir(const std::filesystem::path& dir) {
  try {
    mSkipDirs.push_back(fs::canonical(dir));
  } catch (const fs::filesystem_error&) {
    // Directory does not exist yet; store as-is so it takes effect if created later.
    mSkipDirs.push_back(fs::absolute(dir));
  }
}

bool GitRepository::isTargetPath(const std::filesystem::path& path, bool checkExtension) {
  fs::path canonicalPath;

  // Canonicalize the path (relative paths are resolved relative to sourceRoot).
  try {
    canonicalPath = fs::canonical(path.is_absolute() ? path : (getSourceRoot() / path));
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

  auto relPath = canonicalPath.lexically_relative(mSourceRoot).string();
  auto matcher = [&](const auto& exclude) { return fnmatch(exclude.c_str(), relPath.c_str(), 0) == 0; };
  if (std::any_of(mExcludes.begin(), mExcludes.end(), matcher)) {
    return false;
  }

  if (checkExtension) {
    std::string extension = path.extension();

    if (!mExtensions.empty() &&
        std::find(mExtensions.begin(), mExtensions.end(), extension) == mExtensions.end()) {
      return false;
    }
  }

  return true;
}

void GitRepository::validateRevision(const std::string& rev) const {
  SafeGit2ObjPtr<git_repository, git_repository_free> repo;
  if (git_repository_open_ext(repo.getPtr(), mSourceRoot.c_str(), 0, nullptr) != 0) {
    throw InvalidArgumentException(
        fmt::format("--from: cannot open git repository at '{}'.", mSourceRoot));
  }

  SafeGit2ObjPtr<git_object, git_object_free> obj;
  if (git_revparse_single(obj.getPtr(), static_cast<git_repository*>(repo), rev.c_str()) == 0) {
    return;  // Revision resolves — valid.
  }

  // Revision failed to resolve.  Allow it only if HEAD is a root commit
  // (no parents), since refs like HEAD^ or HEAD~1 are unresolvable in
  // that case but are handled at diff time via empty-tree fallback.
  const auto unresolvedMsg = fmt::format(
      "--from: revision '{}' cannot be resolved. "
      "Verify the revision exists in the repository.", rev);

  SafeGit2ObjPtr<git_object, git_object_free> headObj;
  if (git_revparse_single(headObj.getPtr(), static_cast<git_repository*>(repo), "HEAD") != 0) {
    throw InvalidArgumentException(unresolvedMsg);
  }

  SafeGit2ObjPtr<git_object, git_object_free> headPeeled;
  if (git_object_peel(headPeeled.getPtr(),
                      static_cast<git_object*>(headObj),
                      GIT_OBJECT_COMMIT) != 0) {
    throw InvalidArgumentException(unresolvedMsg);
  }

  if (git_commit_parentcount(headPeeled.cast<git_commit*>()) == 0) {
    return;  // Root commit — parent refs are expected to fail; allow.
  }

  throw InvalidArgumentException(unresolvedMsg);
}

SourceLines GitRepository::getSourceLines(const std::optional<std::string>& from,
                                          bool uncommitted) {
  // Step 1: find all git repos directly at or below sourceRoot (multi-repo support).
  // In Android-style workspaces there is no top-level .git; each component has its own.
  std::vector<fs::path> repoPaths = collectGitRepos(getSourceRoot(), mSkipDirs);

  // Step 2: also probe upward for an enclosing repo.  This handles the case where
  // sourceRoot is a subdirectory inside a single git repo without nested repos.
  // The enclosing repo's workdir is recorded for deduplication below.
  fs::path enclosingWorkdir;
  {
    SafeGit2ObjPtr<git_repository, git_repository_free> probe;
    if (git_repository_open_ext(probe.getPtr(), getSourceRoot().c_str(), 0, nullptr) == 0) {
      const char* rawWorkdir = git_repository_workdir(static_cast<git_repository*>(probe));
      if (rawWorkdir) {
        enclosingWorkdir = fs::canonical(fs::path(rawWorkdir));
      }
    }
  }

  if (repoPaths.empty() && enclosingWorkdir.empty()) {
    throw RepositoryException("Failed to open repository");
  }

  // Track which git workdirs have already been processed to avoid double-counting
  // (e.g. when sourceRoot itself has .git it appears in both repoPaths and enclosingWorkdir).
  std::set<fs::path> processedWorkdirs;
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
      Logger::warn("Failed to open git repo at {}", repoPath);
      continue;
    }

    const char* rawWorkdir = git_repository_workdir(static_cast<git_repository*>(repo));
    if (!rawWorkdir) {
      Logger::warn("Repo at {} has no working directory, skipping", repoPath);
      continue;
    }
    fs::path gitWorkdir = fs::canonical(fs::path(rawWorkdir));
    processedWorkdirs.insert(gitWorkdir);

    std::vector<char*> cstrs;
    git_diff_options opts = buildOpts(cstrs);
    DiffData d(this, gitWorkdir);
    try {
      applyDiff(static_cast<git_repository*>(repo), from, uncommitted, &opts, &d, gitWorkdir);
    } catch (const RepositoryException& e) {
      Logger::warn("diff failed for {}: {}", gitWorkdir, e.what());
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
        fs::path gitWorkdir = fs::canonical(fs::path(rawWorkdir));
        std::vector<char*> cstrs;
        git_diff_options opts = buildOpts(cstrs);
        DiffData d(this, gitWorkdir);
        try {
          applyDiff(static_cast<git_repository*>(repo), from, uncommitted, &opts, &d, gitWorkdir);
        } catch (const RepositoryException& e) {
          if (repoPaths.empty()) {
            throw;  // Only repo available; propagate so the caller sees the error.
          }
          Logger::warn("diff failed for enclosing repo {}: {}", gitWorkdir, e.what());
        }

        SourceLines lines = d.getSourceLines();
        allSourceLines.insert(allSourceLines.end(), lines.begin(), lines.end());
      }
    }
  }

  std::sort(allSourceLines.begin(), allSourceLines.end());
  allSourceLines.erase(std::unique(allSourceLines.begin(), allSourceLines.end()),
                       allSourceLines.end());
  return allSourceLines;
}

std::shared_ptr<SourceTree> GitRepository::getSourceTree() {
  return std::make_shared<GitSourceTree>(getSourceRoot());
}

}  // namespace sentinel
