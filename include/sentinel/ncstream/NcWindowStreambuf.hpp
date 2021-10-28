/*
  MIT License

  Copyright (c) 2021 LG Electronics, Inc.

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


#ifndef INCLUDE_SENTINEL_NCSTREAM_NCWINDOWSTREAMBUF_HPP_
#define INCLUDE_SENTINEL_NCSTREAM_NCWINDOWSTREAMBUF_HPP_

#include <ncurses.h>
#include <cstdint>
#include <iostream>

namespace sentinel {

/**
 * @brief WindowInfo struct storing corner locations of an Ncurses window/pad
 */
struct WindowInfo {
  /**
   * @brief Default constructor.
   */
  WindowInfo() : minX(0), minY(0), maxX(0), maxY(0), height(0), width(0) {}

  /**
   * @brief Default constructor. Param order same with ncurses::window()
   *
   * @param h height
   * @param w width
   * @param y top left y coordinate
   * @param x top left x coordinate
   */
  WindowInfo(int h, int w, int y, int x) :
      minX(x), minY(y), height(h), width(w),
      maxX(x+w-1), maxY(y+h-1) {
  }

  /**
   * @brief top left x coordinate
   */
  int minX;

  /**
   * @brief top left y coordinate
   */
  int minY;

  /**
   * @brief bottom right x coordinate
   */
  int maxX;

  /**
   * @brief bottom right y coordinate
   */
  int maxY;

  /**
   * @brief height
   */
  int height;

  /**
   * @brief width
   */
  int width;
};

namespace ncstream {

/**
 * @brief NcWindowStreambuf class
 */
class NcWindowStreambuf : public std::streambuf {
 public:
  /**
   * @brief Constructor
   *
   * @param os output stream to capture
   * @param o output window
   * @param p output pad
   * @param winInfo position information about p
   * @param scroll bar (param type is pointer to pointer because
   *        redrawing a scrollbar requires free/delete the old window object
   *        and creating new one.
   *        So we need to use pointer to pointer to propagate the change
   *        from NcWindowsStreambuf to CommandGui.)
   * @param scrollInfo position information about scrollbar
   * @param lineCounter pointer to line counter variable
   * @param currPos pointer to the current line number position of cursor in p
   */
  NcWindowStreambuf(
      std::ostream& os, WINDOW* o, WINDOW* p, WindowInfo* winInfo,
      WINDOW** scroll, WindowInfo* scrollInfo,
      int* lineCounter, int* currPos);

  /**
   * @brief Destructor
   */
  virtual ~NcWindowStreambuf();

  /**
   * @brief Sends a character to this object's ncurses panel.
   *
   * @param c character to send
   * @return c on success.
   *         OEF if sync() returns EOF.
   *         ERR if waddch() causes error.
   */
  virtual int overflow(int c);

  /**
   * @brief update panels
   *
   * @return 0 on successs. EOF in case of error.
   */
  virtual int sync();

  /**
   * @brief get line count
   *
   * @return line count
   */
  int GetLineCount() {
    return *mLineCounter;
  }

 private:
  WINDOW* mOutputWin;
  WINDOW* mPad;
  WINDOW** mScroll;
  WindowInfo* mPadInfo;
  WindowInfo* mScrollInfo;

  std::ostream* m_os;
  std::streambuf* m_old;
  int* mLineCounter;
  int* mCurrPos;
  int mCurrLineLength;

  void copy(const NcWindowStreambuf& rhs);
  void refreshOutputWindows();
};

}  // namespace ncstream
}  // namespace sentinel


#endif  // INCLUDE_SENTINEL_NCSTREAM_NCWINDOWSTREAMBUF_HPP_
