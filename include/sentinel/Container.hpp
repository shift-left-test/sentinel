/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_CONTAINER_HPP_
#define INCLUDE_SENTINEL_CONTAINER_HPP_

#include <experimental/filesystem>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <vector>


namespace sentinel {

/**
 * @brief Container template class
 *
 * @tparam T object type
 */
template <typename T>
class Container {
 public:
  /**
   * @brief Default constructor
   */
  Container() = default;

  /**
   * @brief Default constructor
   *
   * @param first iterator
   * @param last iterator
   */
  Container(typename std::vector<T>::iterator first,
            typename std::vector<T>::iterator last);

  /**
   * @brief Return the number of elements
   *
   * @return size
   */
  std::size_t size() const noexcept;

  /**
   * @brief Remove the element specified by the iterator
   *
   * @return the iterator following the last removed element
   */
  typename std::vector<T>::iterator erase(
      typename std::vector<T>::const_iterator pos);

  /**
   * @brief Return the iterator pointing to the first element
   *
   * @return the beginning iterator
   */
  typename std::vector<T>::iterator begin() noexcept;

  /**
   * @brief Return the iterator pointing to the first element
   *
   * @return the beginning iterator
   */
  typename std::vector<T>::const_iterator begin() const noexcept;

  /**
   * @brief Return the iterator pointing to the past-the-end element
   *
   * @return the past-the-end iterator
   */
  typename std::vector<T>::iterator end() noexcept;

  /**
   * @brief Return the iterator pointing to the past-the-end element
   *
   * @return the past-the-end iterator
   */
  typename std::vector<T>::const_iterator end() const noexcept;

  /**
   * @brief Returns a reference to the element at the given position
   *
   * @param position of an element
   * @return reference of the element
   */
  typename std::vector<T>::reference at(std::size_t position);

  /**
   * @brief Returns a reference to the element at the given position
   *
   * @param position of an element
   * @return reference of the element
   */
  typename std::vector<T>::const_reference at(std::size_t position) const;

  /**
   * @brief Returns a reference to the element at the given position
   *
   * @param position of an element
   * @return reference of the element
   */
  typename std::vector<T>::reference operator[](std::size_t position);

  /**
   * @brief Returns a reference to the element at the given position
   *
   * @param position of an element
   * @return reference of the element
   */
  typename std::vector<T>::const_reference
  operator[](std::size_t position) const;

  /**
   * @brief Add the element at the end
   *
   * @param object to add
   */
  void push_back(const typename std::vector<T>::value_type& object);

  /**
   * @brief Add the element at the end
   *
   * @param object to add
   */
  void push_back(typename std::vector<T>::value_type&& object);

  /**
   * @brief Construct and add the end
   *
   * @tparam Args constructor argument types
   * @param args for constructor
   */
  template <class... Args>
  void emplace_back(Args&&... args);

  /**
   * @brief Sort the elements using the comparator
   *
   * @tparam Comparator type
   * @param comparator for sorting
   */
  template <typename Comparator>
  void sort(Comparator comparator);

  /**
   * @brief Sort the elements into ascending order
   */
  void sort();

  /**
   * @brief Shuffle the elements
   */
  void shuffle();

  /**
   * @brief Shuffle the elements
   */
  void shuffle(unsigned randomSeed);

  /**
   * @brief Clear the elements
   */
  void clear() noexcept;

  /**
   * @brief Return the subset of the elements
   *
   * @param first index
   * @param last index
   * @return the subset of the elements
   */
  Container<T> split(std::size_t first, std::size_t last);

  /**
   * @brief Save the elements to the file
   *
   * @param path to file
   */
  virtual void save(const std::string& path);

  /**
   * @brief Load elements from the file
   *
   * @param path to file
   */
  virtual void load(const std::string& path);

 private:
  std::vector<T> mData;
};

template <typename T>
Container<T>::Container(typename std::vector<T>::iterator first,
                        typename std::vector<T>::iterator last) :
    mData(first, last) {
}

template <typename T>
std::size_t Container<T>::size() const noexcept {
  return mData.size();
}

template <typename T>
typename std::vector<T>::iterator Container<T>::erase(
    typename std::vector<T>::const_iterator pos) {
  return mData.erase(pos);
}

template <typename T>
typename std::vector<T>::iterator Container<T>::begin() noexcept {
  return mData.begin();
}

template <typename T>
typename std::vector<T>::const_iterator Container<T>::begin() const noexcept {
  return mData.begin();
}

template <typename T>
typename std::vector<T>::iterator Container<T>::end() noexcept {
  return mData.end();
}

template <typename T>
typename std::vector<T>::const_iterator Container<T>::end() const noexcept {
  return mData.end();
}

template <typename T>
typename std::vector<T>::reference Container<T>::at(std::size_t position) {
  return mData.at(position);
}

template <typename T>
typename std::vector<T>::const_reference
Container<T>::at(std::size_t position) const {
  return mData.at(position);
}

template <typename T>
typename std::vector<T>::reference
Container<T>::operator[](std::size_t position) {
  return mData.at(position);
}

template <typename T>
typename std::vector<T>::const_reference
Container<T>::operator[](std::size_t position) const {
  return mData.at(position);
}

template <typename T>
void Container<T>::push_back(
    const typename std::vector<T>::value_type& object) {
  mData.push_back(object);
}

template <typename T>
void Container<T>::push_back(typename std::vector<T>::value_type&& object) {
  mData.push_back(object);
}

template <typename T>
template <class... Args>
void Container<T>::emplace_back(Args&&... args) {
  mData.emplace_back(args...);
}

template <typename T>
template <typename Comparator>
void Container<T>::sort(Comparator comparator) {
  std::sort(mData.begin(), mData.end(), comparator);
}

template <typename T>
void Container<T>::sort() {
  std::sort(mData.begin(), mData.end());
}

template <typename T>
void Container<T>::shuffle() {
  std::shuffle(mData.begin(), mData.end(),
               std::mt19937 { std::random_device {}() });
}

template <typename T>
void Container<T>::shuffle(unsigned randomSeed) {
  std::shuffle(mData.begin(), mData.end(),
               std::mt19937 { randomSeed });
}

template <typename T>
void Container<T>::clear() noexcept {
  mData.clear();
}

template <typename T>
Container<T> Container<T>::split(std::size_t first,
                                 std::size_t last) {
  return Container<T>(std::next(begin(), first),
                      std::next(begin(), last));
}

template <typename T>
void Container<T>::save(const std::string& path) {
  std::experimental::filesystem::path tmpPath(path);
  auto dirname = tmpPath.parent_path();
  if (!dirname.empty()) {
    std::experimental::filesystem::create_directories(dirname);
  }
  std::ofstream ofs(path);
  for (const auto& data : mData) {
    ofs << data << std::endl;
  }
  ofs.close();
}

template <typename T>
void Container<T>::load(const std::string& path) {
  T object;
  std::ifstream ifs(path);
  while (ifs >> object) {
    mData.push_back(object);
  }
  ifs.close();
}

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_CONTAINER_HPP_
