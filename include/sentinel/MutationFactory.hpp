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

#ifndef INCLUDE_SENTINEL_MUTATIONFACTORY_HPP_
#define INCLUDE_SENTINEL_MUTATIONFACTORY_HPP_

#include <memory>
#include <string>
#include "sentinel/MutableGenerator.hpp"
#include "sentinel/Mutables.hpp"
#include "sentinel/SourceLines.hpp"


namespace sentinel {

/**
 * @brief MutationFactory class
 */
class MutationFactory {
 public:
  /**
   * @brief Default constructor
   *
   * @param generator Mutation generator
   */
  explicit MutationFactory(const std::shared_ptr<MutableGenerator>& generator);

  /**
   * @brief Populate mutables from the given source lines
   *
   * @param gitPath path to git repo
   * @param sourceLines lines of the source
   * @param maxMutables maximum number of mutables generated
   * @return list of mutables
   */
  Mutables populate(const std::string& gitPath,
                    const SourceLines& sourceLines,
                    std::size_t maxMutables);

 private:
  std::shared_ptr<MutableGenerator> mGenerator;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONFACTORY_HPP_
