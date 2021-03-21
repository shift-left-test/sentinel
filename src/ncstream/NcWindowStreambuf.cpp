/*
  MIT License

  Copyright (c) 2021 Loc Duy Phan

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

#include <panel.h>
#include <cstdint>
#include <iostream>
#include <sstream>
#include "sentinel/ncstream/NcWindowStreambuf.hpp"

namespace sentinel {

const int scrollBufferSize = 20000;

ncstream::NcWindowStreambuf::NcWindowStreambuf(
    std::ostream& os, WINDOW* o, WINDOW* p, WindowInfo* winInfo,
    WINDOW** scroll, WindowInfo* scrollInfo,
    int* lineCounter, int* currPos) :
    mOutputWin(o), mPad(p), mPadInfo(winInfo), mLineCounter(lineCounter),
    mScroll(scroll), mScrollInfo(scrollInfo),
    m_os(&os), m_old(os.rdbuf()), mCurrLineLength(0), mCurrPos(currPos) {
  this->setp(nullptr, nullptr);
  this->setg(nullptr, nullptr, nullptr);
  os.rdbuf(this);
  // mvwinch(p, p->_maxy, 0 );
  mvwinch(p, 0, 0);
}

void ncstream::NcWindowStreambuf::copy(const NcWindowStreambuf& rhs) {
  if (this != &rhs) {
    this->mPad = rhs.mPad;
    this->m_os = rhs.m_os;
    this->m_old = rhs.m_old;
  }
}

ncstream::NcWindowStreambuf::~NcWindowStreambuf() {
  if (this->m_os != nullptr) {
    this->m_os->rdbuf(this->m_old);
  }
}

int ncstream::NcWindowStreambuf::overflow(int c) {
  int ret = c;
  if (c != EOF) {
    if (mCurrLineLength == mPadInfo->width) {
      *mLineCounter = std::min(scrollBufferSize, *mLineCounter+1);
      mCurrLineLength = 0;
    }

    if (ERR == waddch(this->mPad, static_cast<chtype>(c))) {
      ret = EOF;
    } else {
      mCurrLineLength++;
      if (c == '\n') {
        *mLineCounter = std::min(scrollBufferSize, *mLineCounter+1);
        mCurrLineLength = 0;
        refreshOutputWindows();
      }
    }
  } else {
    refreshOutputWindows();
  }

  return ret;
}

int ncstream::NcWindowStreambuf::sync() {
  return 0;
}

void ncstream::NcWindowStreambuf::refreshOutputWindows() {
  *mCurrPos = std::max(0, *mLineCounter - mPadInfo->height);
  int scrollLength = mScrollInfo->height;
  int numLinesPerPixel = *mLineCounter / scrollLength;
  if (*mLineCounter % scrollLength != 0) {
    numLinesPerPixel++;
  }
  int scrollBarSize = std::max(1, scrollLength / numLinesPerPixel);
  int maxScrollbarStartPos = scrollLength - scrollBarSize;
  int startPos = std::min(*mCurrPos / numLinesPerPixel,
                          maxScrollbarStartPos);

  // clear old scroll bar
  wbkgd(*mScroll, COLOR_PAIR(0));
  wrefresh(*mScroll);
  delwin(*mScroll);

  // draw new scrol bar
  *mScroll = subwin(mOutputWin, scrollBarSize, mScrollInfo->width,
                    mScrollInfo->minY+startPos, mScrollInfo->minX);
  wbkgd(*mScroll, COLOR_PAIR(1));
  wrefresh(*mScroll);

  // refresh output pad
  prefresh(mPad, *mCurrPos, 0,
           mPadInfo->minY, mPadInfo->minX,
           mPadInfo->maxY, mPadInfo->maxX);
}

}  // namespace sentinel
