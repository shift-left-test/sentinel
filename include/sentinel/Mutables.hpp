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

#ifndef INCLUDE_SENTINEL_MUTABLES_HPP_
#define INCLUDE_SENTINEL_MUTABLES_HPP_

#include <vector>
#include <string>
#include "sentinel/Mutable.hpp"
#include "sentinel/Persistence.hpp"


namespace sentinel {

/**
 * @brief Mutables class
 */
class Mutables : public Persistence {
 public:
  /**
   * @brief Default constructor
   *
   * @param path to the mutables
   */
  explicit Mutables(const std::string& path);

  /**
   * @brief Add the given mutable to the object
   *
   * @param m mutable object
   */
  void add(const Mutable& m);

  /**
   * @brief Return a mutable object at the given index
   *
   * @param index for the object
   * @return Mutable object
   */
  Mutable get(std::size_t index);

  /**
   * @brief Return number of stored Mutable
   *
   * @return number of stored Mutable
   */
  int size();

  /**
   * @brief Return the iterator to the first Mutable in Mutables.
   *
   * @return iterator to the first Mutable.
   */
  inline std::vector<Mutable>::const_iterator begin() const {
    return mData.cbegin();
  }

  /**
   * @brief Return the iterator to the last Mutable in Mutables.
   *
   * @return iterator to the last Mutable.
   */
  inline std::vector<Mutable>::const_iterator end() const {
    return mData.cend();
  }

  /**
   * @brief Read a string literal from binary file.
   *
   * @return string read from file.
   */
  static std::string readStringFromFile(std::ifstream& inFile);

  /**
   * @brief Read an integer from binary file.
   *
   * @return int read from file.
   */
  static int readIntFromFile(std::ifstream& inFile);

  void load() override;
  void save() override;

 private:
  std::string mPath;
  std::vector<Mutable> mData;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_MUTABLES_HPP_
