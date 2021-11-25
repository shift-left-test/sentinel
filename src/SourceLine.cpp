/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <string>
#include "sentinel/SourceLine.hpp"


namespace sentinel {

SourceLine::SourceLine(const std::experimental::filesystem::path& path,
    std::size_t lineNumber) :
    mPath(path), mLineNumber(lineNumber) {
}

}  // namespace sentinel
