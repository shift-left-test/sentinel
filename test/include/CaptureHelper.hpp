/*
  MIT License

  Copyright (c) 2020 LG Electronics, Inc.

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
