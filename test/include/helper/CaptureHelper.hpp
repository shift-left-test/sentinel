/*
 * Copyright (c) 2020 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef TEST_INCLUDE_CAPTUREHELPER_HPP_
#define TEST_INCLUDE_CAPTUREHELPER_HPP_


#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>


namespace sentinel {

class CaptureHelper {
 public:
  static std::shared_ptr<CaptureHelper> getStdoutCapture() {
    return std::shared_ptr<CaptureHelper>(new CaptureHelper(&std::cout));
  }

  static std::shared_ptr<CaptureHelper> getStderrCapture() {
    return std::shared_ptr<CaptureHelper>(new CaptureHelper(&std::cerr));
  }

  ~CaptureHelper() {
    release();
  }

  void capture() {
    if (!mIsCapturing) {
      mOriginalBuffer = mStream->rdbuf();
      mStream->rdbuf(mRedirectStream.rdbuf());
      mIsCapturing = true;
    }
  }

  std::string release() {
    if (mIsCapturing) {
      std::string wOutput = mRedirectStream.str();
      mRedirectStream.str(std::string());
      mStream->rdbuf(mOriginalBuffer);
      mIsCapturing = false;
      return wOutput;
    }
    return "";
  }

 private:
  explicit CaptureHelper(std::ostream* ioStream)
    : mStream(ioStream), mIsCapturing(false), mOriginalBuffer(nullptr) {
  }

  std::ostream* mStream;
  bool mIsCapturing;
  std::stringstream mRedirectStream;
  std::streambuf* mOriginalBuffer;
};

}  // namespace sentinel

#endif  // TEST_INCLUDE_CAPTUREHELPER_HPP_
