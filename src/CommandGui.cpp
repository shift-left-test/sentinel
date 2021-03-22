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

#include <fmt/core.h>
#include <form.h>
#include <ncurses.h>
#include <panel.h>
#include <term.h>
#include <unistd.h>
#include <sys/wait.h>
#include <experimental/filesystem>
#include <algorithm>
#include <exception>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include "sentinel/exceptions/InvalidArgumentException.hpp"
#include "sentinel/Logger.hpp"
#include "sentinel/ncstream/term.hpp"
#include "sentinel/GitRepository.hpp"
#include "sentinel/util/string.hpp"
#include "sentinel/util/signal.hpp"
#include "sentinel/util/Subprocess.hpp"
#include "sentinel/Evaluator.hpp"
#include "sentinel/XMLReport.hpp"
#include "sentinel/HTMLReport.hpp"
#include "sentinel/CommandGui.hpp"

namespace sentinel {

const int MINLINES = 24;
const int MINCOLS = 80;
static const char* cCommandGuiLoggerName = "CommandGui";
const int configWinHeight = 15;
const int advancedWinWidth = 100;
const int advancedWinHeight = 15;
static const char* guiHelp = R"a1b2z(F5: Start  F1: Exit  F3: Toggle Advanced Options  CtrlC: Force Stop)a1b2z";
std::vector<const char*> verboseValues = {
    "<    true    >",
    "<    false   >",
    nullptr};
std::vector<const char*> generatorValues = {
    "<   uniform  >",
    "<  weighted  >",
    "<   random   >",
    nullptr};
std::vector<const char*> scopeValues = {
    "<     all    >",
    "<   commit   >",
    nullptr};
static std::streambuf* oldCerrStreambuf;
static std::streambuf* oldCoutStreambuf;
TERMINAL* term = nullptr;

static void guiSignalHandler(int signum) {
  namespace fs = std::experimental::filesystem;
  if (!workDirExists) {
    fs::remove_all(workDirForSH);
  } else {
    if (!backupDirExists) {
      fs::remove_all(workDirForSH / "backup");
    }
    if (!expectedDirExists) {
      fs::remove_all(workDirForSH / "expected");
    }
    if (!actualDirExists) {
      fs::remove_all(workDirForSH / "actual");
    }
  }
  std::cout.flush();
  if (signum != SIGUSR1) {
    if (term != nullptr) {
      set_curterm(term);
    }
    endwin();
    std::cerr.rdbuf(oldCerrStreambuf);
    std::cout.rdbuf(oldCoutStreambuf);
    std::cerr <<
      fmt::format("Receive a signal({}).", strsignal(signum)) << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

CommandGui::CommandGui(args::Subparser& parser) :
    titleWin(nullptr), configWin(nullptr), outputWin(nullptr), helpPad(nullptr),
    outputPad(nullptr), basicForm(nullptr), helpWin(nullptr),
    basicFields(34, nullptr), advancedFields(22, nullptr),
    outputScroll(nullptr), advancedForm(nullptr),
    advancedWin(nullptr), advancedPanel(nullptr), titlePanel(nullptr),
    outputPadCurrSize(1), outputPadCurrPos(0), outputPadScrollBufferSize(20000),
    currConfigPage(1), numBasicConfigPages(1), configScrollbarSize(1),
    CommandRun(parser) {
  basicOptions.push_back(std::vector<std::string>{
      "Source Root Path:",
      R"asdf(Source Root Directory
This directory should be an existing git directory (has .git folder) containing target source files for mutation testing.)asdf",
      mSourceRoot.Get()});
  basicOptions.push_back(std::vector<std::string>{
      "Build Directory:",
      R"asdf(Build Directory
This directory must contain compile_commands.json, which is the compilation database of the target project.
The build and test command shall be executed from this directory.)asdf",
      mBuildDir.Get()});
  basicOptions.push_back(std::vector<std::string>{
      "Output Directory:",
      R"asdf(Output Directory
Directory contains html files showing mutation testing detailed results.)asdf",
      mOutputDir.Get()});
  basicOptions.push_back(std::vector<std::string>{
      "Test Result Directory:",
      R"asdf(Test Command Output Directory
Directory contains the test output files generated after test command execution.
The directory must be empty before starting mutation testing.)asdf",
      mTestResultDir.Get()});
  basicOptions.push_back(std::vector<std::string>{
      "Build Command:",
      R"asdf(Build Command
Shell command to build the target project from given build directory.)asdf",
      mBuildCmd.Get()});
  basicOptions.push_back(std::vector<std::string>{
      "Test Command:",
      R"asdf(Test Command
Shell command to execute test cases from given build directory.)asdf",
      mTestCmd.Get()});

  advancedOptions.push_back(std::vector<std::string>{
      "Working Directory:",
      R"asdf(Working Directory
Sentinel temporary working directory contains:
- test execution results of original program
- test execution results of mutated program
- copies of original version of targeted files)asdf",
      mWorkDir.Get()});
  advancedOptions.push_back(std::vector<std::string>{
      "Test Result Extensions:",
      R"asdf(Test Result Extensions
SENTINEL currently can only handle Gtest xml format test result files.)asdf",
      string::join(",", mTestResultFileExts.Get())});
  advancedOptions.push_back(std::vector<std::string>{
      "Target File Extensions:",
      R"asdf(Target File Extensions
Extentions of source files to be mutated.
SENTINEL currently support C/C++.)asdf",
      string::join(",", mExtensions.Get())});
  advancedOptions.push_back(std::vector<std::string>{
      "Excluded File/Path:",
      R"asdf(Excluded Paths
Files/paths excluded from mutation testing.
It is recommended not to mutate test files and third party, external source files.)asdf",
      string::join(",", mExcludes.Get())});
  advancedOptions.push_back(std::vector<std::string>{
      "Mutant Limit:",
      R"asdf(Mutant Limit
Maximum number of mutants to be generated.)asdf",
      std::to_string(mLimit.Get())});
  advancedOptions.push_back(std::vector<std::string>{
      "Test Timeout:",
      R"asdf(Test Timeout
Time limit (in seconds) for test execution of the mutated program.
By default, the time limit is automatically set to be the same as that of the original program.)asdf",
      mTimeLimitStr.Get()});
  advancedOptions.push_back(std::vector<std::string>{
      "Send SIGKILL After:",
      R"asdf(Send SIGKILL if test-command is still running after timeout. If 0, SIGKILL is not sent. This option has no meaning when timeout is set 0.)asdf",
      mKillAfterStr.Get()});
  advancedOptions.push_back(std::vector<std::string>{
      "Random Seed:",
      R"asdf(Random Seed
Used for debugging, experiments.)asdf",
      std::to_string(mSeed.Get())});
  advancedOptions.push_back(std::vector<std::string>{
      "Mutant Generator:",
      R"asdf(Mutant Generator
- uniform: target lines are randomly selected, and 1 mutant (if any) is randomly generated on each line until number of generated mutants reaches limit.
- weighted: similar to uniform generator, but target lines are selected based on statement depth. Deeper stmt are selected first.
- random: mutants are selected randomly.)asdf", "<   uniform  >"});
  advancedOptions.push_back(std::vector<std::string>{
      "Scope:", R"asdf(Scope
- all: all files in the source root path are targeted, except for new, unstaged files.
- commit: only source lines that are changed in the latest commit are targeted.)asdf", "<     all    >"});
  advancedOptions.push_back(std::vector<std::string>{
      "Verbose:", R"asdf(Verbosity)asdf", "<    true    >"});
}

void CommandGui::setSignalHandler() {
  signal::setMultipleSignalHandlers({SIGABRT, SIGINT, SIGFPE, SIGILL, SIGSEGV,
      SIGTERM, SIGQUIT, SIGHUP, SIGUSR1}, guiSignalHandler);
}

int CommandGui::run() {
  oldCerrStreambuf = std::cerr.rdbuf();
  oldCoutStreambuf = std::cout.rdbuf();
  auto logger = Logger::getLogger(cCommandGuiLoggerName);
  logger->setLevel(Logger::Level::INFO);

  try {
    initGui();
    ncstream::NcWindowStreambuf coutBuf(
        std::cout, outputWin, outputPad, &outputPadInfo,
        &outputScroll, &outputScrollInfo,
        &outputPadCurrSize, &outputPadCurrPos);
    ncstream::NcWindowStreambuf cerrBuf(
        std::cerr, outputWin, outputPad, &outputPadInfo,
        &outputScroll, &outputScrollInfo,
        &outputPadCurrSize, &outputPadCurrPos);
    handleUserInteraction();
    quitGui();
  } catch(const std::exception& ex) {
    quitGui();
    std::cerr.rdbuf(oldCerrStreambuf);
    std::cout.rdbuf(oldCoutStreambuf);
    logger->error(fmt::format("{0}", ex.what()));
  } catch(...) {
    quitGui();
    std::cerr.rdbuf(oldCerrStreambuf);
    std::cout.rdbuf(oldCerrStreambuf);
    logger->error("Unknown error.");
  }

  return 0;
}

void CommandGui::initGui() {
  initscr();
  cbreak();
  noecho();
  start_color();
  keypad(stdscr, TRUE);

  // Check if current window size satisfy minimum requirement
  if (LINES < MINLINES || COLS < MINCOLS) {
    throw InvalidArgumentException(
        fmt::format(
            "Screen size should be larger than {}x{}. (Current: {}x{})",
            std::to_string(MINLINES), std::to_string(MINCOLS), LINES, COLS));
  }

  init_pair(0, COLOR_WHITE, COLOR_BLACK);
  init_pair(1, COLOR_WHITE, COLOR_WHITE);
  init_pair(2, COLOR_WHITE, COLOR_BLUE);

  // Create SENTINEL title window
  int titleWinHeight = LINES;
  int titleWinWidth = COLS;
  titleWinInfo = WindowInfo(titleWinHeight, titleWinWidth, 0, 0);
  titleWin = newwin(titleWinHeight, titleWinWidth, 0, 0);
  box(titleWin, 0, 0);
  mvwprintw(titleWin, 1, titleWinWidth/2-4, "SENTINEL");
  mvwprintw(titleWin, titleWinInfo.maxY-1,
            titleWinWidth/2-strlen(guiHelp)/2, guiHelp);

  // Create configuration window
  int helpWinWidth = COLS / 8 * 3;
  int configWinWidth = titleWinWidth - 2 - helpWinWidth;
  configWinInfo = WindowInfo(configWinHeight, configWinWidth,
                             2, helpWinWidth+1);
  configWin = subwin(
      titleWin, configWinInfo.height, configWinInfo.width,
      configWinInfo.minY, configWinInfo.minX);
  box(configWin, 0, 0);
  mvwprintw(configWin, 0, configWinInfo.width/2-6, "Basic Options");

  // Create basic configuration fields
  int currY = 0;
  int currX = 1;
  int labelWidth = 24;
  int numBasicOptions = basicOptions.size();

  for (int i = 0; i < numBasicOptions*2; i++, currY += 2) {
    basicFields[i] = new_field(1, basicOptions[i/2][0].length(),
                               currY, currX, 0, 0);
    field_opts_on(basicFields[i], O_VISIBLE);
    field_opts_off(basicFields[i], O_EDIT);
    field_opts_off(basicFields[i], O_ACTIVE);
    set_field_buffer(basicFields[i], 0, basicOptions[i/2][0].c_str());

    if (currY >= configWinHeight-3) {
      currY = 0;
      set_new_page(basicFields[i], TRUE);
      move_field(basicFields[i], currY, currX);
      numBasicConfigPages++;
    }

    ++i;
    basicFields[i] = new_field(1, configWinWidth-2-labelWidth-4,
                               currY, currX+labelWidth, 0, 0);
    set_field_back(basicFields[i], A_UNDERLINE);
    field_opts_on(basicFields[i], O_VISIBLE);
    field_opts_on(basicFields[i], O_ACTIVE);
    field_opts_on(basicFields[i], O_EDIT);
    field_opts_off(basicFields[i], O_STATIC);
    field_opts_off(basicFields[i], O_AUTOSKIP);
    set_field_buffer(basicFields[i], 0, basicOptions[i/2][2].c_str());
  }
  set_field_back(basicFields[0], A_STANDOUT);

  // Create advanced configuration fields
  currY = 0;
  currX = 1;
  int numAdvancedOptions = advancedOptions.size();

  for (int i = 0; i < numAdvancedOptions*2; i++, currY += 1) {
    advancedFields[i] = new_field(1, advancedOptions[i/2][0].length(),
                                  currY, currX, 0, 0);
    field_opts_on(advancedFields[i], O_VISIBLE);
    field_opts_off(advancedFields[i], O_EDIT);
    field_opts_off(advancedFields[i], O_ACTIVE);
    set_field_buffer(advancedFields[i], 0, advancedOptions[i/2][0].c_str());

    ++i;
    advancedFields[i] = new_field(1, configWinWidth-2-labelWidth-4,
                                  currY, currX+labelWidth, 0, 0);
    set_field_back(advancedFields[i], A_UNDERLINE);
    field_opts_on(advancedFields[i], O_VISIBLE);
    field_opts_on(advancedFields[i], O_ACTIVE);
    field_opts_on(advancedFields[i], O_EDIT);
    field_opts_off(advancedFields[i], O_AUTOSKIP);
    field_opts_off(advancedFields[i], O_STATIC);
    set_field_buffer(advancedFields[i], 0, advancedOptions[i/2][2].c_str());
  }
  set_field_back(advancedFields[0], A_STANDOUT);

  set_field_back(advancedFields[17], A_NORMAL);
  set_field_back(advancedFields[19], A_NORMAL);
  set_field_back(advancedFields[21], A_NORMAL);
  field_opts_off(advancedFields[17], O_EDIT);
  field_opts_off(advancedFields[19], O_EDIT);
  field_opts_off(advancedFields[21], O_EDIT);
  set_field_type(advancedFields[17], TYPE_ENUM, generatorValues.data(), 0, 1);
  set_field_type(advancedFields[19], TYPE_ENUM, scopeValues.data(), 0, 1);
  set_field_type(advancedFields[21], TYPE_ENUM, verboseValues.data(), 0, 1);

  // Set up basic form
  basicForm = new_form(basicFields.data());
  advancedForm = new_form(advancedFields.data());
  set_form_win(basicForm, configWin);
  set_form_sub(basicForm,
               derwin(configWin, configWinHeight-3, configWinWidth-4, 2, 1));
  post_form(basicForm);

  // Create help window
  int helpWinHeight = configWinHeight;
  helpWinInfo = WindowInfo(helpWinHeight, helpWinWidth, 2, 1);
  helpWin = subwin(
      titleWin, helpWinInfo.height, helpWinInfo.width,
      helpWinInfo.minY, helpWinInfo.minX);
  box(helpWin, 0, 0);
  mvwprintw(helpWin, 0, helpWinInfo.width/2-2, "Help");

  // Create an inner window to print help messages
  helpPadInfo = WindowInfo(helpWinHeight-3, helpWinWidth-4, 2, 2);
  helpPad = derwin(
      helpWin, helpPadInfo.height, helpPadInfo.width,
      helpPadInfo.minY, helpPadInfo.minX);
  mvwprintw(helpPad, 0, 0, basicOptions[0][1].c_str());

  // Create output window
  int outputWinHeight = LINES - configWinInfo.minY - configWinInfo.height - 2;
  int outputWinWidth = titleWinWidth - 2;
  outputWinInfo = WindowInfo(outputWinHeight, outputWinWidth,
                             configWinInfo.minY+configWinInfo.height, 1);
  outputWin = subwin(
      titleWin, outputWinInfo.height, outputWinInfo.width,
      outputWinInfo.minY, outputWinInfo.minX);
  box(outputWin, 0, 0);
  mvwprintw(outputWin, 0, outputWinWidth/2-3, "Output");

  // Create output pad inside outputWin to print
  int outputPadHeight = outputWinHeight - 2;
  int outputPadWidth = outputWinWidth - 7;

  // outputPadInfo stores the area in which the pad is shown
  outputPadInfo = WindowInfo(outputPadHeight, outputPadWidth,
                             outputWinInfo.minY+1, 4);
  outputPad = newpad(outputPadScrollBufferSize, outputPadWidth);
  scrollok(outputPad, true);

  // Create scrollbar for output window
  mvwaddch(outputWin, 1, outputWinInfo.width-3, '^');
  mvwaddch(outputWin, outputWinInfo.height-2, outputWinInfo.width-3, 'v');
  outputScrollInfo = WindowInfo(
      outputPadHeight-2, 1, outputPadInfo.minY+1, outputPadInfo.maxX+2);
  outputScroll = subwin(
      outputWin, outputScrollInfo.height, outputScrollInfo.width,
      outputScrollInfo.minY, outputScrollInfo.minX);
  wbkgd(outputScroll, COLOR_PAIR(1));

  // Create advanced window
  advancedWinInfo = WindowInfo(
      configWinInfo.height, configWinInfo.width,
      configWinInfo.minY, configWinInfo.minX);
  advancedWin = newwin(
      advancedWinInfo.height, advancedWinInfo.width,
      advancedWinInfo.minY, advancedWinInfo.minX);
  box(advancedWin, 0, 0);
  mvwprintw(advancedWin, 0, advancedWinInfo.width/2-7, "Advanced Options");

  // create advanced config window to contain the form
  set_form_win(advancedForm, advancedWin);
  set_form_sub(advancedForm,
               derwin(advancedWin, advancedWinInfo.height-3,
                      advancedWinInfo.width-4, 2, 1));
  post_form(advancedForm);

  // refresh all windows
  refresh();
  wrefresh(configWin);

  titlePanel = new_panel(titleWin);
  advancedPanel = new_panel(advancedWin);
  hide_panel(advancedPanel);
  update_panels();
  doupdate();
}

void CommandGui::handleUserInteraction() {
  // Register handling of left mouse click.
  mousemask(BUTTON1_CLICKED, nullptr);
  auto logger = Logger::getLogger(cCommandGuiLoggerName);
  logger->setLevel(Logger::Level::INFO);

  int ch;
  MEVENT event;
  bool fillConfig = true;
  bool advancedWinOn = false;

  while ((ch = getch()) != KEY_F(1)) {
    switch (ch) {
      case '\t':
        if (advancedWinOn) {
          selectNextField(advancedForm);
          break;
        }

        if (fillConfig) {
          selectNextField(basicForm);
        }
        break;
      case KEY_DOWN:
        if (advancedWinOn) {
          selectNextField(advancedForm);
          break;
        }

        if (fillConfig) {
          selectNextField(basicForm);
        } else {
          if (outputPadCurrPos + 1 < outputPadCurrSize) {
            outputPadCurrPos++;
            refreshOutputWin();
          }
        }
        break;
      case KEY_UP:
        if (advancedWinOn) {
          selectPrevField(advancedForm);
          break;
        }

        if (fillConfig) {
          selectPrevField(basicForm);
        } else {
          if (outputPadCurrPos > 0) {
            outputPadCurrPos--;
            refreshOutputWin();
          }
        }
        break;
      case KEY_NPAGE:
        if (advancedWinOn) {
          break;
        }

        if (fillConfig) {
        } else {
          outputPadCurrPos = std::min(
              outputPadCurrSize-1, outputPadCurrPos+outputPadInfo.height);
          refreshOutputWin();
        }
        break;
      case KEY_PPAGE:
        if (advancedWinOn) {
          break;
        }

        if (fillConfig) {
        } else {
          outputPadCurrPos =
              std::max(0, outputPadCurrPos-outputPadInfo.height);
          refreshOutputWin();
        }
        break;
      case KEY_LEFT:
        if (advancedWinOn) {
          int currFieldIdx = field_index(current_field(advancedForm));
          if (currFieldIdx >= 17 && currFieldIdx <= 21) {
            form_driver(advancedForm, REQ_PREV_CHOICE);
          } else {
            form_driver(advancedForm, REQ_PREV_CHAR);
          }
          break;
        }

        if (fillConfig) {
          form_driver(basicForm, REQ_PREV_CHAR);
        }
        break;
      case KEY_RIGHT:
        if (advancedWinOn) {
          int currFieldIdx = field_index(current_field(advancedForm));
          if (currFieldIdx >= 17 && currFieldIdx <= 21) {
            form_driver(advancedForm, REQ_NEXT_CHOICE);
          } else {
            form_driver(advancedForm, REQ_NEXT_CHAR);
          }
          break;
        }

        if (fillConfig) {
          form_driver(basicForm, REQ_NEXT_CHAR);
        }
        break;
      case KEY_BACKSPACE:
      case 127:
      case '\b':
        if (advancedWinOn) {
          int cursorX, cursorY;
          getyx(advancedWin, cursorY, cursorX);
          if (cursorX > 26) {
            // Scroll back (in case field too long), delete a char
            if (form_driver(advancedForm, REQ_SCR_BCHAR) == E_OK) {
              form_driver(advancedForm, REQ_DEL_CHAR);
            } else {
              form_driver(advancedForm, REQ_DEL_PREV);
            }
          }
          break;
        }

        if (fillConfig) {
          int cursorX, cursorY;
          getyx(configWin, cursorY, cursorX);
          if (cursorX > 26) {
            // Scroll back (in case field too long), delete a char
            if (form_driver(basicForm, REQ_SCR_BCHAR) == E_OK) {
              form_driver(basicForm, REQ_DEL_CHAR);
            } else {
              form_driver(basicForm, REQ_DEL_PREV);
            }
          }
        }
        break;
      case KEY_HOME:
        if (advancedWinOn) {
          form_driver(advancedForm, REQ_BEG_FIELD);
          break;
        }

        if (fillConfig) {
          form_driver(basicForm, REQ_BEG_FIELD);
        } else {
          outputPadCurrPos = 0;
          refreshOutputWin();
        }
        break;
      case KEY_END:
        if (advancedWinOn) {
          form_driver(advancedForm, REQ_END_FIELD);
          break;
        }

        if (fillConfig) {
          form_driver(basicForm, REQ_END_FIELD);
        } else {
          outputPadCurrPos =
              std::max(0, outputPadCurrSize-outputPadInfo.height);
          refreshOutputWin();
        }
        break;
      case KEY_DC:
        if (advancedWinOn) {
          form_driver(advancedForm, REQ_DEL_CHAR);
          break;
        }

        if (fillConfig) {
          form_driver(basicForm, REQ_DEL_CHAR);
        }
        break;
      case KEY_F(3):
        // Toggle Advanced option window
        if (advancedWinOn) {
          advancedWinOn = false;
          fillConfig = true;
          hide_panel(advancedPanel);
          update_panels();
          doupdate();
          refreshHelpWin(basicForm);
          wrefresh(configWin);
        } else {
          advancedWinOn = true;
          curs_set(1);
          show_panel(advancedPanel);
          update_panels();
          doupdate();
          refreshHelpWin(advancedForm);
        }
        break;
      case KEY_F(5):
        // Suggestion: add a confirm prompt window when user wants to start.

        // Start mutation testing
        // reset output pad
        wclear(outputPad);
        outputPadCurrPos = 0;
        outputPadCurrSize = 1;
        fillConfig = false;

        // Refresh the form to include newly filled field.
        // Without this, field value is not updated with click start
        form_driver(basicForm, REQ_VALIDATION);
        form_driver(advancedForm, REQ_VALIDATION);
        wrefresh(outputPad);

        try {
          // Change logging default level based on option
          if (getVerbose()) {
            sentinel::Logger::setDefaultLevel(sentinel::Logger::Level::INFO);
          } else {
            sentinel::Logger::setDefaultLevel(sentinel::Logger::Level::OFF);
          }

          CommandRun::run();
        } catch(const std::exception& ex) {
          logger->error(fmt::format("Error occurred: {0}", ex.what()));
        } catch (...) {
          logger->error("Unknown Exception.");
        }
        break;
      case KEY_MOUSE:
        if (advancedWinOn) {
          break;
        }

        if (getmouse(&event) == OK) {
          // int mouseX = event.x;
          int mouseY = event.y;

          if (mouseY >= outputWinInfo.minY) {
            curs_set(0);
            prefresh(outputPad, outputPadCurrPos, 0,
                     outputPadInfo.minY, outputPadInfo.minX,
                     outputPadInfo.maxY, outputPadInfo.maxX);
            // Show cursor at end of output pad
            if (outputPadCurrPos >= outputPadCurrSize-outputPadInfo.height) {
              curs_set(1);
            }
            fillConfig = false;
          } else {
            wrefresh(configWin);
            curs_set(1);
            fillConfig = true;
          }
        }
        break;
      default:
        if (advancedWinOn) {
          form_driver(advancedForm, ch);
          break;
        }

        if (fillConfig) {
          form_driver(basicForm, ch);
        }
        break;
    }

    if (advancedWinOn) {
      wrefresh(advancedWin);
      continue;
    }

    if (fillConfig) {
      wrefresh(configWin);
    }
  }
}

void CommandGui::quitGui() {
  unpost_form(basicForm);
  for (auto& field : basicFields) {
    free_field(field);
  }
  unpost_form(advancedForm);
  for (auto& field : advancedFields) {
    free_field(field);
  }
  free_form(basicForm);
  free_form(advancedForm);
  delwin(helpWin);
  delwin(configWin);
  delwin(titleWin);
  delwin(outputScroll);
  delwin(outputPad);
  delwin(outputWin);
  delwin(advancedWin);
  endwin();
}

void CommandGui::selectNextField(FORM* form) {
  int currFieldLabelIdx = field_index(current_field(form))-1;
  std::vector<FIELD*>* fields = nullptr;

  if (form == basicForm) {
    fields = &basicFields;
  } else {
    fields = &advancedFields;
  }
  set_field_back(fields->at(currFieldLabelIdx), A_NORMAL);

  form_driver(form, REQ_NEXT_FIELD);
  form_driver(form, REQ_END_LINE);

  currFieldLabelIdx = field_index(current_field(form))-1;
  set_field_back(fields->at(currFieldLabelIdx), A_STANDOUT);
  refreshHelpWin(form);
}

void CommandGui::selectPrevField(FORM* form) {
  int currFieldLabelIdx = field_index(current_field(form))-1;
  std::vector<FIELD*>* fields = nullptr;

  if (form == basicForm) {
    fields = &basicFields;
  } else {
    fields = &advancedFields;
  }
  set_field_back(fields->at(currFieldLabelIdx), A_NORMAL);

  form_driver(form, REQ_PREV_FIELD);
  form_driver(form, REQ_END_LINE);

  currFieldLabelIdx = field_index(current_field(form))-1;
  set_field_back(fields->at(currFieldLabelIdx), A_STANDOUT);
  refreshHelpWin(form);
}

void CommandGui::refreshOutputWin() {
  refreshOutputScrollbar();
  prefresh(outputPad, outputPadCurrPos, 0,
           outputPadInfo.minY, outputPadInfo.minX,
           outputPadInfo.maxY, outputPadInfo.maxX);

  // Show cursor at the end of output pad
  if (outputPadCurrPos >= outputPadCurrSize-outputPadInfo.height) {
    curs_set(1);
  } else {
    curs_set(0);
  }
}

void CommandGui::refreshOutputScrollbar() {
  int scrollLength = outputScrollInfo.height;
  int numLinesPerPixel = outputPadCurrSize / scrollLength;
  if (outputPadCurrSize % scrollLength != 0) {
    numLinesPerPixel++;
  }
  int scrollbarSize = std::max(1, scrollLength / numLinesPerPixel);
  int maxScrollbarStartPos = scrollLength - scrollbarSize;
  int startPos = std::min(outputPadCurrPos / numLinesPerPixel,
                          maxScrollbarStartPos);

  // clear old scroll bar
  wbkgd(outputScroll, COLOR_PAIR(0));
  wrefresh(outputScroll);
  delwin(outputScroll);

  // draw new scroll bar
  outputScroll = subwin(
    outputWin, scrollbarSize, outputScrollInfo.width,
    outputScrollInfo.minY+startPos, outputScrollInfo.minX);
  wbkgd(outputScroll, COLOR_PAIR(1));
  wrefresh(outputScroll);
}

void CommandGui::refreshHelpWin(FORM* form) {
  wclear(helpPad);
  int currFieldIdx = field_index(current_field(form)) / 2;
  if (form == basicForm) {
    mvwprintw(helpPad, 0, 0, basicOptions[currFieldIdx][1].c_str());
  } else {
    mvwprintw(helpPad, 0, 0, advancedOptions[currFieldIdx][1].c_str());
  }
  wrefresh(helpPad);
  wrefresh(form_win(form));
}

std::string CommandGui::getSourceRoot() {
  return string::trim(std::string(field_buffer(basicFields[1], 0)));
}

std::string CommandGui::getBuildDir() {
  return string::trim(std::string(field_buffer(basicFields[3], 0)));
}

std::string CommandGui::getOutputDir() {
  return string::trim(std::string(field_buffer(basicFields[5], 0)));
}

std::string CommandGui::getTestResultDir() {
  return string::trim(std::string(field_buffer(basicFields[7], 0)));
}

std::string CommandGui::getBuildCmd() {
  return string::trim(std::string(field_buffer(basicFields[9], 0)));
}

std::string CommandGui::getTestCmd() {
  return string::trim(std::string(field_buffer(basicFields[11], 0)));
}

std::string CommandGui::getWorkDir() {
  return string::trim(std::string(field_buffer(advancedFields[1], 0)));
}

std::vector<std::string> CommandGui::getTestResultFileExts() {
  return string::split(
      string::trim(std::string(field_buffer(advancedFields[3], 0))), ',');
}

std::vector<std::string> CommandGui::getTargetFileExts() {
  return string::split(
      string::trim(std::string(field_buffer(advancedFields[5], 0))), ',');
}

std::vector<std::string> CommandGui::getExcludePaths() {
  return string::split(
      string::trim(std::string(field_buffer(advancedFields[7], 0))), ',');
}

int CommandGui::getMutantLimit() {
  return std::stoul(
      string::trim(std::string(field_buffer(advancedFields[9], 0))));
}

std::string CommandGui::getTestTimeLimit() {
  return  string::trim(std::string(field_buffer(advancedFields[11], 0)));
}

std::string CommandGui::getKillAfter() {
  return string::trim(std::string(field_buffer(advancedFields[13], 0)));
}

unsigned CommandGui::getSeed() {
  return std::stoul(
      string::trim(std::string(field_buffer(advancedFields[15], 0))));
}

std::string CommandGui::getGenerator() {
  std::string gen{field_buffer(advancedFields[17], 0)};
  gen = string::trim(gen.substr(1, 12));
  return gen;
}

std::string CommandGui::getScope() {
  std::string scope{field_buffer(advancedFields[19], 0)};
  scope = string::trim(scope.substr(1, 12));
  return scope;
}

bool CommandGui::getVerbose() {
  std::string verbosity{field_buffer(advancedFields[21], 0)};
  verbosity = string::trim(verbosity.substr(1, 12));
  return string::stringToBool(verbosity);
}

}  // namespace sentinel
