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

#ifndef INCLUDE_SENTINEL_OPERATOR_MUTATIONOPERATOR_HPP_
#define INCLUDE_SENTINEL_OPERATOR_MUTATIONOPERATOR_HPP_

#include <string>
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "sentinel/Mutables.hpp"

namespace sentinel {

/**
 * @brief MutationOperator abstract class
 */
class MutationOperator {
 public:
  /**
   * @brief Default constructor
   *
   * @param name of mutation operator
   * @param CI Clang compiler management object
   */
  MutationOperator(std::string name, clang::CompilerInstance& CI)
      : name(name), mCI(CI), mSrcMgr(CI.getSourceManager()) {
  }

  /**
   * @brief Return True if this mutation operator can be applied to give AST
   *        node. Consider the domain of the mutation operator as well as
   *        the AST node's surrounding context to avoid generating stillborn
   *        mutants.
   *
   * @param s target AST node
   */
  virtual bool canMutate(clang::Stmt* s) = 0;

  /**
   * @brief Create Mutable from given statement.
   *
   * @param s target AST node
   * @param mutables to store newly created mutable
   */
  virtual void populate(clang::Stmt* s, Mutables* mutables) = 0;

 protected:
  /**
   * @brief name of mutation operator
   */
  std::string name;
  
  /**
   * @brief Clang compiler management object
   */
  clang::CompilerInstance& mCI;

  /**
   * @brief Object handles loading and caching of source files into memory
   */
  clang::SourceManager& mSrcMgr;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_OPERATOR_MUTATIONOPERATOR_HPP__