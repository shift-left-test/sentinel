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

#ifndef INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_
#define INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_

#include <vector>
#include <string>
#include <queue>
#include "sentinel/MutationResult.hpp"
#include "sentinel/Persistence.hpp"


namespace sentinel {

/**
 * @brief MutationResults class
 */
class MutationResults : public Persistence {
 public:
  /**
   * @brief Default constructor
   *
   * @param path to the MutationResults
   * 
   * @throw InvalidArgumentException when path is not directory
   */
  explicit MutationResults(const std::string& path);

  /**
   * @brief Add the given MutationResult to the object
   *
   * @param m MutationResult object
   */
  void add(const MutationResult& m);

  /**
   * @brief Return a MutationResult object at the given index
   *
   * @param index for the object
   * @return MutationResult object
   */
  MutationResult get(std::size_t index);

  /**
   * @brief Return number of stored MutationResult
   *
   * @return number of stored MutationResult
   */
  int size();

  /**
   * @brief sort By Index of Mutable DB 
   *
   */
  void sortByIndexOfMutableDB();


  /**
   * @brief Return the iterator to the first MutationResult in MutationResults.
   *
   * @return iterator to the first MutationResult.
   */
  inline std::vector<MutationResult>::const_iterator begin() const {
    return mData.cbegin();
  }

  /**
   * @brief Return the iterator to the last MutationResult in MutationResults.
   *
   * @return iterator to the last MutationResult.
   */
  inline std::vector<MutationResult>::const_iterator end() const {
    return mData.cend();
  }

  /**
   * @brief load from path
   *
   * @throw InvalidArgumentException when path is empty
   */
  void load() override;

  /**
   * @brief save to path
   */
  void save() override;

 private:
  std::string mPath;
  std::vector<MutationResult> mData;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTATIONRESULTS_HPP_
