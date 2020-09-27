 /*
  MIT License

  Copyright (c) 2020 Sangmo Kang

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

#ifndef INCLUDE_SENTINEL_DOCGENERATOR_CSSGENERATOR_HPP_
#define INCLUDE_SENTINEL_DOCGENERATOR_CSSGENERATOR_HPP_

#include <stddef.h>
#include <string>
#include <vector>
#include "sentinel/docGenerator/DOCGenerator.hpp"

namespace sentinel {

/**
 * @brief CSSGenerator class
 */
class CSSGenerator : public DOCGenerator {
 public:
  /**
   * @brief make css string
   */
  inline std::string str() override {
    return styleCssContent;
  }

 private:
  std::string styleCssContent =
      R"(html, body, div, span, p, blockquote, pre {
    margin: 0;
    padding: 0;
    border: 0;
    outline: 0;
    font-weight: inherit;
    font-style: inherit;
    font-size: 100%;
    font-family: inherit;
    vertical-align: baseline;
}

body{
    line-height: 1;
    color: black;
    background: white;
    margin-left: 20px;
}

.src { 
    border: 1px solid #dddddd;
    padding-top: 10px;
    padding-right: 5px;
    padding-left: 5px;
    font-family: Consolas, Courier, monospace;
}

.covered {
    background-color: #ddffdd;
}

.uncovered {
    background-color: #ffdddd;
}

.killed, .KILLED {
    background-color: #aaffaa;
}

.survived, .SURVIVED {
    background-color: #ffaaaa;
}

.uncertain {
    background-color: #dde7ef;
}

.run_error {
    background-color: #dde7ef;
}

.na {
    background-color: #eeeeee;
}

.timed_out {
    background-color: #dde7ef;
}

.non_viable {
    background-color: #aaffaa;
}

.memory_error {
    background-color: #dde7ef;
}

.not_started {
    background-color: #dde7ef; color : red
}

.no_coverage {
    background-color: #ffaaaa;
}

.tests {
    width: 50%;
    float: left;
}

.mutees {
    float: right;
    width: 50%;
}

.unit {
    padding-top: 20px;
    clear: both;
}

.coverage_bar {
    display: inline-block;
    height: 1.1em;
    width: 130px;
    background: #FAA;
    margin: 0 5px;
    vertical-align: middle;
    border: 1px solid #AAA;
    position: relative;
}

.coverage_complete {
    display: inline-block;
    height: 100%;
    background: #DFD;
    float: left;
}

.coverage_legend {
    position: absolute;
    height: 100%;
    width: 100%;
    left: 0;
    top: 0;
    text-align: center;
}

.line, .mut {
    vertical-align: middle;
}

.coverage_percentage {
    display: inline-block;
    width: 3em;
    text-align: right;
}

.pop {
    outline:none;
}

.pop strong {
    line-height: 30px;
}

.pop {
    text-decoration: none;
}

.pop span {
    z-index: 10;
    display: none;
    padding: 14px 20px;
    margin-top: -30px;
    margin-left: 28px;
    width: 800px;
    line-height: 16px;
    word-wrap: break-word;
    border-radius: 4px;
    -moz-border-radius: 4px;
    -webkit-border-radius: 4px;
    -moz-box-shadow: 5px 5px 8px #CCC;
    -webkit-box-shadow: 5px 5px 8px #CCC;
    box-shadow: 5px 5px 8px #CCC;
}

.pop:hover span {
    display: inline;
    position: absolute;
    color: #111;
    border: 1px solid #DCA;
    background: #fffAF0;
}

.width-1 {
    width: 1%;
}

.width-2 {
    width: 2%;
}

.width-3 {
    width: 3%;
}

.width-4 {
    width: 4%;
}

.width-5 {
    width: 5%;
}

.width-6 {
    width: 6%;
}

.width-7 {
    width: 7%;
}

.width-8 {
    width: 8%;
}

.width-9 {
    width: 9%;
}

.width-10 {
    width: 10%;
}

.width-11 {
    width: 11%;
}

.width-12 {
    width: 12%;
}

.width-13 {
    width: 13%;
}

.width-14 {
    width: 14%;
}

.width-15 {
    width: 15%;
}

.width-16 {
    width: 16%;
}

.width-17 {
    width: 17%;
}

.width-18 {
    width: 18%;
}

.width-19 {
    width: 19%;
}

.width-20 {
    width: 20%;
}

.width-21 {
    width: 21%;
}

.width-22 {
    width: 22%;
}

.width-23 {
    width: 23%;
}

.width-24 {
    width: 24%;
}

.width-25 {
    width: 25%;
}

.width-26 {
    width: 26%;
}

.width-27 {
    width: 27%;
}

.width-28 {
    width: 28%;
}

.width-29 {
    width: 29%;
}

.width-30 {
    width: 30%;
}

.width-31 {
    width: 31%;
}

.width-32 {
    width: 32%;
}

.width-33 {
    width: 33%;
}

.width-34 {
    width: 34%;
}

.width-35 {
    width: 35%;
}

.width-36 {
    width: 36%;
}

.width-37 {
    width: 37%;
}

.width-38 {
    width: 38%;
}

.width-39 {
    width: 39%;
}

.width-40 {
    width: 40%;
}

.width-41 {
    width: 41%;
}

.width-42 {
    width: 42%;
}

.width-43 {
    width: 43%;
}

.width-44 {
    width: 44%;
}

.width-45 {
    width: 45%;
}

.width-46 {
    width: 46%;
}

.width-47 {
    width: 47%;
}

.width-48 {
    width: 48%;
}

.width-49 {
    width: 49%;
}

.width-50 {
    width: 50%;
}

.width-51 {
    width: 51%;
}

.width-52 {
    width: 52%;
}

.width-53 {
    width: 53%;
}

.width-54 {
    width: 54%;
}

.width-55 {
    width: 55%;
}

.width-56 {
    width: 56%;
}

.width-57 {
    width: 57%;
}

.width-58 {
    width: 58%;
}

.width-59 {
    width: 59%;
}

.width-60 {
    width: 60%;
}

.width-61 {
    width: 61%;
}

.width-62 {
    width: 62%;
}

.width-63 {
    width: 63%;
}

.width-64 {
    width: 64%;
}

.width-65 {
    width: 65%;
}

.width-66 {
    width: 66%;
}

.width-67 {
    width: 67%;
}

.width-68 {
    width: 68%;
}

.width-69 {
    width: 69%;
}

.width-70 {
    width: 70%;
}

.width-71 {
    width: 71%;
}

.width-72 {
    width: 72%;
}

.width-73 {
    width: 73%;
}

.width-74 {
    width: 74%;
}

.width-75 {
    width: 75%;
}

.width-76 {
    width: 76%;
}

.width-77 {
    width: 77%;
}

.width-78 {
    width: 78%;
}

.width-79 {
    width: 79%;
}

.width-80 {
    width: 80%;
}

.width-81 {
    width: 81%;
}

.width-82 {
    width: 82%;
}

.width-83 {
    width: 83%;
}

.width-84 {
    width: 84%;
}

.width-85 {
    width: 85%;
}

.width-86 {
    width: 86%;
}

.width-87 {
    width: 87%;
}

.width-88 {
    width: 88%;
}

.width-89 {
    width: 89%;
}

.width-90 {
    width: 90%;
}

.width-91 {
    width: 91%;
}

.width-92 {
    width: 92%;
}

.width-93 {
    width: 93%;
}

.width-94 {
    width: 94%;
}

.width-95 {
    width: 95%;
}

.width-96 {
    width: 96%;
}

.width-97 {
    width: 97%;
}

.width-98 {
    width: 98%;
}

.width-99 {
    width: 99%;
}

.width-100 {
    width: 100%;
})";
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_DOCGENERATOR_CSSGENERATOR_HPP_
