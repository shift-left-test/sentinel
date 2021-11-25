/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <fmt/core.h>
#include <string>
#include "sentinel/docGenerator/IndexHTMLGenerator.hpp"
#include "sentinel/util/string.hpp"

namespace sentinel {
IndexHTMLGenerator::IndexHTMLGenerator(bool root,
                                       const std::string& dirName,
                                      std::size_t sizeOfTargetFiles,
                                      unsigned int coverage,
                                      std::size_t numerator,
                                      std::size_t denominator) :
                                      mRoot(root),
                                      mDirName(dirName),
                                      mSizeOfTargetFiles(sizeOfTargetFiles),
                                      mCoverage(coverage),
                                      mNumerator(numerator),
                                      mDenominator(denominator) {
}

void IndexHTMLGenerator::pushItemToTable(const std::string& subName,
                                         int subCoverage,
                                         std::size_t subNumerator,
                                         std::size_t subDenominator,
                                         std::size_t numOfFiles) {
  std::string subPath;
  if (mRoot) {
    subPath = fmt::format("./srcDir/{0}index.html",
                          subName.empty() ? "" : subName + "/");
  } else {
    subPath = fmt::format("./{0}.html", subName);
  }
  mTableItem +=
      fmt::format(mRoot ? indexRootTableItem : indexSubTableItem,
                  fmt::arg("sub_path", subPath),
                  fmt::arg("sub_name", subName.empty() ? "." : subName),
                  fmt::arg("num_of_files", numOfFiles),
                  fmt::arg("sub_cov", subCoverage),
                  fmt::arg("sub_numerator", subNumerator),
                  fmt::arg("sub_denominator", subDenominator));
}

std::string IndexHTMLGenerator::str() {
  std::string indexTitle = mRoot ? indexRootTitle : indexSubTitle;
  if (!mRoot) {
    indexTitle = fmt::format(indexTitle, fmt::arg("dir_name",
                             mDirName.empty() ? "." : mDirName));
  }
  auto tableCol = mRoot ? indexRootTableCol : indexSubTableCol;
  return fmt::format(indexSkeleton,
                     fmt::arg("style_path", mRoot ? "" :
                              mDirName.empty() ? "../" : "../../"),
                     fmt::arg("index_title", indexTitle),
                     fmt::arg("size_of_target_files", mSizeOfTargetFiles),
                     fmt::arg("cov", mCoverage),
                     fmt::arg("numerator", mNumerator),
                     fmt::arg("denominator", mDenominator),
                     fmt::arg("Directory_or_File", mRoot ?
                              "Directory" : "File"),
                     fmt::arg("table_col", tableCol),
                     fmt::arg("table_item", mTableItem),
                     fmt::arg("https", "https://"));
}

}  // namespace sentinel
