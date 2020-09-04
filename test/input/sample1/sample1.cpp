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

// #include <iostream>

// Macro test: with and without semicolon (;)
#define IS_NEGATIVE(a) a < 0
#define INVALID_RECT_SIDE_ERR -1;

inline bool lessThanOrEqual(int a, int b) {
  return a <= b;
}

// Return surface area of a rectangular
int rectangularSurfaceArea(const int h, const float l, double w) {
  // LCR ROR UOI test
  // SBR test: if, return
  if (IS_NEGATIVE(h) || IS_NEGATIVE(l) || w < 0)
    return INVALID_RECT_SIDE_ERR

  // Comment test: middle of code
  // AOR test: basic mutation + modulo mutation test
  // UOI test: no const variable mutation
  return 2 * (l * h + w * h/*+ l * l*/+ w * l);
}

// Pointer operation test
int charArraySizie(const char *ptr_start, const char *ptr_end) {
  return (ptr_end - ptr_start) / sizeof(char);
}

int sumOfEvenPositiveNumber(int from, int to) {
  int ret = 0;
  int i = from;

  for (; lessThanOrEqual(i, to); ++i) {
    if ((i & 1) == (1 << 0) && i > 0) {
      ret = ret + i;
    }
  }

  return ret;
}

