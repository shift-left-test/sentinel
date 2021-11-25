/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <git2.h>
#include <algorithm>
#include <sstream>
#include "sentinel/GitRepository.hpp"
#include "sentinel/GitSourceTree.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/util/string.hpp"
#include "sentinel/exceptions/InvalidArgumentException.hpp"


namespace sentinel {

const char * cGitRepositoryLoggerName = "GitRepository";
/**
 * @brief git_xxx pointer free guard
 */
template<typename T, void Free(T*)>
class SafeGit2ObjPtr {
 private:
  T * obj;

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
  T ** getPtr() { return &obj; }

  /**
   * @brief cast to T pointer
   */
  explicit operator T*() { return obj; }

  /**
   * @brief cast to C
   */
  template<typename C>
  C cast() {
    return reinterpret_cast<C>(obj); // NOLINT
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
   */
  explicit DiffData(GitRepository * gitRepo) : mGitRepo(gitRepo) {}

  /**
   * @brief add Source Line
   *
   * @param file source file name
   *
   * @param line line number
   */
  void addSourceLine(const std::string & file, int line) {
    namespace fs = std::experimental::filesystem;

    fs::path filePath(mGitRepo->getSourceRoot());
    filePath.append(file);
    if (mGitRepo->isTargetPath(filePath)) {
      mSourceLines.push_back(SourceLine(filePath, line));
    }
  }

  /**
   * @brief get source lines
   */
  SourceLines getSourceLines() { return mSourceLines; }

 private:
  /// diff result
  SourceLines mSourceLines;
  /// logger
  GitRepository * mGitRepo;
};

static void getDiffFromTree(git_repository* repo, git_tree* tree,
  DiffData& d) {  // NOLINT
  SafeGit2ObjPtr<git_diff, git_diff_free> diff;

  if (git_diff_tree_to_workdir_with_index(diff.getPtr(),
                                          static_cast<git_repository*>(repo),
                                          static_cast<git_tree*>(tree),
                                          nullptr) != 0) {
    throw RepositoryException("Failed to find diff");
  }

  if (git_diff_foreach(static_cast<git_diff*>(diff),
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
    [](const git_diff_delta* delta,
    const git_diff_hunk* hunk,
    const git_diff_line* line,
    void *payload) {
      // each_line_cb
      auto d = reinterpret_cast<DiffData*>(payload); // NOLINT
      auto logger = Logger::getLogger(cGitRepositoryLoggerName);
      logger->debug(fmt::format("{}:{}:{}",
                               line->origin,
                               delta->new_file.path,
                               line->new_lineno));

      if (line->origin == '+') {
        d->addSourceLine(delta->new_file.path, line->new_lineno);
      }
      return 0;
    },
    &d) != 0) {
    throw RepositoryException("Failed to diff iterate");
  }
}

GitRepository::GitRepository(const std::string& path,
  const std::vector<std::string>& extensions,
  const std::vector<std::string>& excludes) {
  namespace fs = std::experimental::filesystem;

  git_libgit2_init();

  auto logger = Logger::getLogger(cGitRepositoryLoggerName);

  try {
    mSourceRoot = fs::canonical(path);
  } catch(const fs::filesystem_error & e) {
    throw InvalidArgumentException(fmt::format("source_root option error: {}",
        e.what()));
  }
  logger->info(fmt::format("source root: {}", mSourceRoot.string()));

  if (!extensions.empty()) {
    std::transform(extensions.begin(), extensions.end(),
                   std::back_inserter(mExtensions),
                   [](auto extension) -> std::string
                   { return extension.at(0) == '.' ?
                   extension : fmt::format(".{}", extension); });
  }

  try {
    for (const auto& filename : excludes) {
      fs::canonical(filename, mSourceRoot);
    }
  } catch(const fs::filesystem_error& e) {
    throw InvalidArgumentException(fmt::format("exclude option error: {}",
        e.what()));
  }
  this->mExcludes = excludes;
}

GitRepository::~GitRepository() {
  git_libgit2_shutdown();
}

bool GitRepository::isTargetPath(
  const std::experimental::filesystem::path &path, bool checkExtension) {
  namespace fs = std::experimental::filesystem;

  auto logger = Logger::getLogger(cGitRepositoryLoggerName);

  if (!this->mExcludes.empty()) {
    fs::path canonicalPath;
    try {
      canonicalPath = fs::canonical(path, getSourceRoot());
    } catch(const fs::filesystem_error & e) {
      return false;
    }

    for (auto& exclude : this->mExcludes) {
      try {
        fs::path excludePath = fs::canonical(exclude, getSourceRoot());

        auto excludeMatchResult = std::mismatch(
          excludePath.begin(), excludePath.end(),
          canonicalPath.begin(), canonicalPath.end());
        if (excludeMatchResult.first == excludePath.end()) {
          return false;
        }
      } catch (const fs::filesystem_error & e) {
        logger->warn(fmt::format("exclude check error: {}", e.what()));
      }
    }
  }

  if (checkExtension) {
    std::string extension = path.extension();

    if (!this->mExtensions.empty() &&
      std::find(this->mExtensions.begin(), this->mExtensions.end(),
        extension) == this->mExtensions.end()) {
      return false;
    }
  }

  return true;
}

SourceLines GitRepository::getSourceLines(const std::string& scope) {
  auto logger = Logger::getLogger(cGitRepositoryLoggerName);

  SafeGit2ObjPtr<git_repository, git_repository_free> repo;
  if (git_repository_open(repo.getPtr(), getSourceRoot().c_str()) != 0) {
    throw RepositoryException("Fail to open repository");
  }

  DiffData d(this);

  if (scope == "commit") {
    SafeGit2ObjPtr<git_object, git_object_free> obj;
    SafeGit2ObjPtr<git_tree, git_tree_free> tree;
    SafeGit2ObjPtr<git_reference, git_reference_free> ref;
    if (git_reference_lookup(ref.getPtr(), static_cast<git_repository*>(repo),
                            "refs/tags/devtool-base") == 0) {
      logger->info("found devtool tag");
      int error_code = git_reference_peel(
          obj.getPtr(),
          static_cast<git_reference*>(ref), GIT_OBJ_COMMIT);
      if (error_code != 0) {
        auto e = fmt::format("Failed to peel tag: error code {}", error_code);
        throw RepositoryException(e);
      }
    } else {
      git_revparse_single(obj.getPtr(), static_cast<git_repository*>(repo),
                          "HEAD^{commit}");
    }

    if (!obj.isNull()) {
      SafeGit2ObjPtr<git_commit, git_commit_free> parent_commit;

      if (git_commit_parentcount(obj.cast<const git_commit*>()) > 0) {
        if (git_commit_parent(parent_commit.getPtr(),
                              obj.cast<const git_commit*>(), 0) != 0) {
          throw RepositoryException("Failed to find parent commit");
        }

        if (git_commit_tree(tree.getPtr(),
                            static_cast<git_commit*>(parent_commit)) != 0) {
          throw RepositoryException("Failed to find parent tree");
        }
      }
    } else {
      logger->warn("No HEAD commit or devtool tag found");
    }

    getDiffFromTree(static_cast<git_repository*>(repo),
                    static_cast<git_tree*>(tree),
                    d);
  } else if (scope == "all") {
    getDiffFromTree(static_cast<git_repository*>(repo),
                    nullptr,
                    d);
  } else {
    throw InvalidArgumentException(
      fmt::format("scope '{}' is invalid.", scope));
  }

  return d.getSourceLines();
}

std::shared_ptr<SourceTree> GitRepository::getSourceTree() {
  return std::make_shared<GitSourceTree>(getSourceRoot());
}

}  // namespace sentinel
