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

#ifndef INCLUDE_SENTINEL_XMLREPORT_HPP_
#define INCLUDE_SENTINEL_XMLREPORT_HPP_

#include <string>
#include "sentinel/Mutables.hpp"
#include "sentinel/MutationResults.hpp"
#include "sentinel/Report.hpp"
#include "sentinel/SourceTree.hpp"


namespace sentinel {

/**
 * @brief XMLReport class
 */
class XMLReport : public Report {
 public:
  /**
   * @brief Default constructor
   *
   * @param sourceTree source tree object
   * @param mutables list of mutables
   * @param results list of mutation results
   */
  XMLReport(const SourceTree& sourceTree,
            const Mutables& mutables,
            const MutationResults& results);

  void save(const std::string& path) override;

 private:
  SourceTree mSourceTree;
  Mutables mMutables;
  MutationResults mResults;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_XMLREPORT_HPP_