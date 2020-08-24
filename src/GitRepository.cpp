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

#include <git2.h>
#include <sstream>
#include "sentinel/GitRepository.hpp"
#include "sentinel/DefaultLogger.hpp"

namespace sentinel {

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
  operator T*() { return obj; }

  /**
   * @brief cast to C
   */
  template<typename C>
  C cast() {
    return reinterpret_cast<C>(obj);
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
struct DiffData {
  /// diff result
  SourceLines sourceLines;
  /// logger
  std::shared_ptr<Logger> logger = nullptr;
};

RepositoryException::RepositoryException(const std::string &msg) {
  this->msg = msg;
}

GitRepository::GitRepository(const std::string& path)
: path_(path), logger_(std::make_shared<DefaultLogger>("GitRepository")) {
  git_libgit2_init();
}

GitRepository::~GitRepository() {
  git_libgit2_shutdown();
}

SourceLines GitRepository::getSourceLines() {
  SafeGit2ObjPtr<git_repository, git_repository_free> repo;
  SafeGit2ObjPtr<git_object, git_object_free> obj;
  SafeGit2ObjPtr<git_tree, git_tree_free> tree;
  SafeGit2ObjPtr<git_diff, git_diff_free> diff;
  DiffData d;
  d.logger = this->logger_;

  if (git_repository_open(repo.getPtr(), path_.c_str()) != 0) {
    throw RepositoryException("Fail to opne repository");
  }

  SafeGit2ObjPtr<git_reference, git_reference_free> ref;
  if (git_reference_lookup(ref.getPtr(), repo, "refs/tags/devtool-base") == 0) {
    std::stringstream msg;
    msg << "found devtool tag" << std::endl;
    this->logger_->info(msg.str());

    int error_code = git_reference_peel(obj.getPtr(),
    ref, GIT_OBJ_COMMIT);
    if (error_code != 0) {
      throw RepositoryException("Fail to peel tag: error_code"
      + std::to_string(error_code));
    }
  } else {
    git_revparse_single(obj.getPtr(), repo, "HEAD^{commit}");
  }

  if (!obj.isNull()) {
    SafeGit2ObjPtr<git_commit, git_commit_free> parent_commit;

    if (git_commit_parentcount(obj.cast<const git_commit*>()) > 0) {
      if (git_commit_parent(parent_commit.getPtr(),
        obj.cast<const git_commit*>(), 0) != 0) {
        throw RepositoryException("Fail to find parent commit");
      }

      if (git_commit_tree(tree.getPtr(), parent_commit) != 0) {
        throw RepositoryException("Failt to find parent tree");
      }
    }
  } else {
    std::stringstream msg;
    msg << "NO HEAD commit or devtool tag found";
    this->logger_->warn(msg.str());
  }

  if (git_diff_tree_to_workdir_with_index(diff.getPtr(),
    repo, tree, nullptr) != 0) {
    throw RepositoryException("Fail to find diff");
  }

  if (git_diff_foreach(diff,
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
      auto d = reinterpret_cast<DiffData*>(payload);
      if (d->logger) {
        std::ostringstream msg;
        msg << line->origin << ":" << delta->new_file.path
          << ":" << line->new_lineno;
        d->logger->info(msg.str());
      }

      if (line->origin == '+') {
        d->sourceLines.push_back(
          SourceLine(delta->new_file.path, line->new_lineno));
      }
      return 0;
    },
    &d) != 0) {
    throw RepositoryException("Fail to diff iterate");
  }

  return d.sourceLines;
}

std::shared_ptr<SourceTree> GitRepository::getSourceTree() {
  return nullptr;
}

}  // namespace sentinel
