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
const int promptWinHeight = 5;
const int promptWinWidth = 50;
static const char* guiHelp = R"a1b2z(F5: Start  ESC: Exit  F3: Toggle Advanced Options  Ctrl-C: Force Stop)a1b2z";
static const char* exitPrompt = "Are you sure you want to exit? [y/N]";
static const char* startPrompt ="Do you want to start mutation testing? [y/N]";
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

  fs::path backupDir = workDirForSH / "backup";
  if (fs::exists(backupDir)) {
    if (fs::is_directory(backupDir)) {
      CommandRun::restoreBackup(backupDir, sourceRootForSH);
    }
  }
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
    advancedWin(nullptr), exitWin(nullptr), startWin(nullptr),
    exitPanel(nullptr), advancedPanel(nullptr), titlePanel(nullptr),
    startPanel(nullptr),
    outputPadCurrSize(1), outputPadCurrPos(0), outputPadScrollBufferSize(20000),
    currConfigPage(1), numBasicConfigPages(1), configScrollbarSize(1),
    CommandRun(parser) {
  basicOptions.push_back(std::vector<std::string>{
      "Source Root Path:",
      R"asdf(Source Root Path

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

Directory contains html files showing mutation testing detailed results.
If output directory is not given, by default, no output files are generated.)asdf",
      mOutputDir.Get()});
  basicOptions.push_back(std::vector<std::string>{
      "Test Result Directory:",
      R"asdf(Test Result Directory

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
      R"asdf(Excluded File/Path

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
      R"asdf(Send SIGKILL After:

Send SIGKILL if test-command is still running after timeout. If 0, SIGKILL is not sent. This option has no meaning when timeout is set 0.)asdf",
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
      "Verbose:", R"asdf(Verbose)asdf", "<    true    >"});
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
  init_pair(2, COLOR_BLACK, COLOR_WHITE);

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
    field_opts_off(basicFields[i], O_BLANK);
    set_field_buffer(basicFields[i], 0, basicOptions[i/2][2].c_str());

    // Attach to each field a structure to keep track of its value's length
    auto ptr = static_cast<FieldAttrs*>(calloc(1, sizeof(FieldAttrs)));  // NOLINT
    ptr->length = basicOptions[i/2][2].length();
    set_field_userptr(basicFields[i], static_cast<void*>(ptr));
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
    field_opts_off(advancedFields[i], O_BLANK);
    set_field_buffer(advancedFields[i], 0, advancedOptions[i/2][2].c_str());

    // Attach to each field a structure to keep track of its value's length
    auto ptr = static_cast<FieldAttrs*>(calloc(1, sizeof(FieldAttrs)));  // NOLINT
    ptr->length = advancedOptions[i/2][2].length();
    set_field_userptr(advancedFields[i], static_cast<void*>(ptr));
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

  // create exit prompt window
  exitWinInfo = WindowInfo(
      promptWinHeight, promptWinWidth,
      LINES/2-promptWinHeight/2, COLS/2-promptWinWidth/2);
  exitWin = newwin(
      exitWinInfo.height, exitWinInfo.width,
      exitWinInfo.minY, exitWinInfo.minX);
  box(exitWin, 0, 0);
  mvwprintw(exitWin, 2, exitWinInfo.width/2-strlen(exitPrompt)/2, exitPrompt);
  wbkgd(exitWin, COLOR_PAIR(2));

  // create start prompt window
  startWinInfo = WindowInfo(
      promptWinHeight, promptWinWidth,
      LINES/2-promptWinHeight/2, COLS/2-promptWinWidth/2);
  startWin = newwin(
      startWinInfo.height, startWinInfo.width,
      startWinInfo.minY, startWinInfo.minX);
  box(startWin, 0, 0);
  mvwprintw(startWin, 2,
            startWinInfo.width/2-strlen(startPrompt)/2, startPrompt);
  wbkgd(startWin, COLOR_PAIR(2));

  // refresh all windows
  refresh();
  wrefresh(configWin);

  titlePanel = new_panel(titleWin);
  advancedPanel = new_panel(advancedWin);
  exitPanel = new_panel(exitWin);
  startPanel = new_panel(startWin);
  hide_panel(advancedPanel);
  hide_panel(exitPanel);
  hide_panel(startPanel);
  update_panels();
  doupdate();
}

void CommandGui::handleUserInteraction() {
  // Register handling of left mouse click.
  mousemask(BUTTON1_CLICKED, nullptr);
  auto logger = Logger::getLogger(cCommandGuiLoggerName);
  logger->setLevel(Logger::Level::INFO);

  MEVENT event;
  bool fillConfig = true;
  bool advancedWinOn = false;
  bool confirmExit = false;
  bool confirmStart = false;
  bool done = false;

  while (!done) {
    int ch = getch();

    switch (ch) {
      case 27:  // ESCAPE
        if (confirmStart) {
          break;
        }

        if (confirmExit) {
          hide_panel(exitPanel);
          update_panels();
          doupdate();
          curs_set(1);
          confirmExit = false;
          refreshOutputWin();
        } else {
          show_panel(exitPanel);
          update_panels();
          doupdate();
          curs_set(0);
          confirmExit = true;
        }
        // done = true;
        break;
      case '\t':
        if (confirmExit || confirmStart) {
          break;
        }

        if (advancedWinOn) {
          selectNextField(advancedForm);
          break;
        }

        if (fillConfig) {
          selectNextField(basicForm);
        }
        break;
      case KEY_DOWN:
        if (confirmExit || confirmStart) {
          break;
        }

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
        if (confirmExit || confirmStart) {
          break;
        }

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
        if (confirmExit || confirmStart) {
          break;
        }

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
        if (confirmExit || confirmStart) {
          break;
        }

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
        if (confirmExit || confirmStart) {
          break;
        }

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
        if (confirmExit || confirmStart) {
          break;
        }

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
        if (confirmExit || confirmStart) {
          break;
        }

        if (advancedWinOn) {
          int cursorX, cursorY;
          getyx(advancedWin, cursorY, cursorX);
          if (cursorX > 26) {
            // Scroll back (in case field too long), delete a char
            if (form_driver(advancedForm, REQ_SCR_BCHAR) == E_OK) {
              if (form_driver(advancedForm, REQ_DEL_CHAR) == E_OK) {
                updateFieldValueLength(advancedForm, REQ_DEL_CHAR);
              }
            } else {
              if (form_driver(advancedForm, REQ_DEL_PREV) == E_OK) {
                updateFieldValueLength(advancedForm, REQ_DEL_PREV);
              }
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
              if (form_driver(basicForm, REQ_DEL_CHAR) == E_OK) {
                updateFieldValueLength(basicForm, REQ_DEL_CHAR);
              }
            } else {
              if (form_driver(basicForm, REQ_DEL_PREV) == E_OK) {
                updateFieldValueLength(basicForm, REQ_DEL_PREV);
              }
            }
          }
        }
        break;
      case KEY_HOME:
        if (confirmExit || confirmStart) {
          break;
        }

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
        if (confirmExit || confirmStart) {
          break;
        }

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
        if (confirmExit || confirmStart) {
          break;
        }

        if (advancedWinOn) {
          if (form_driver(advancedForm, REQ_DEL_CHAR) == E_OK) {
            updateFieldValueLength(advancedForm, REQ_DEL_CHAR);
          }
          break;
        }

        if (fillConfig) {
          if (form_driver(basicForm, REQ_DEL_CHAR) == E_OK) {
            updateFieldValueLength(basicForm, REQ_DEL_CHAR);
          }
        }
        break;
      case KEY_F(3):
        if (confirmExit || confirmStart) {
          break;
        }

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
        if (confirmExit || confirmStart) {
          break;
        }

        show_panel(startPanel);
        update_panels();
        doupdate();
        curs_set(0);
        confirmStart = true;

        break;
      case KEY_MOUSE:
        if (confirmExit || confirmStart) {
          break;
        }

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
        if (confirmExit) {
          if (ch == 'y') {
            done = true;
          }

          if (ch == 'n' || ch == 'N') {
            hide_panel(exitPanel);
            update_panels();
            doupdate();
            curs_set(1);
            refreshOutputWin();
            confirmExit = false;
          }

          break;
        }

        if (confirmStart) {
          if (ch == 'y') {
            // hide prompt window
            hide_panel(startPanel);
            update_panels();
            doupdate();
            curs_set(1);
            confirmStart = false;

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
                sentinel::Logger::setDefaultLevel(
                    sentinel::Logger::Level::INFO);
              } else {
                sentinel::Logger::setDefaultLevel(
                    sentinel::Logger::Level::OFF);
              }

              CommandRun::run();
            } catch(const std::exception& ex) {
              logger->error(fmt::format("Error occurred: {0}", ex.what()));
            } catch (...) {
              logger->error("Unknown Exception.");
            }
          }

          if (ch == 'n' || ch == 'N') {
            // hide prompt window
            hide_panel(startPanel);
            update_panels();
            doupdate();
            curs_set(1);
            refreshOutputWin();
            confirmStart = false;
          }

          break;
        }

        if (advancedWinOn) {
          if (form_driver(advancedForm, ch) == E_OK) {
            updateFieldValueLength(advancedForm, ch);
          }
          break;
        }

        if (fillConfig) {
          if (form_driver(basicForm, ch) == E_OK) {
            updateFieldValueLength(basicForm, ch);
          }
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

void CommandGui::updateFieldValueLength(FORM* form, int action) {
  auto ptr = static_cast<FieldAttrs*>(field_userptr(current_field(form)));
  int curLen = ptr->length;
  int curPos = form->curcol;

  // Assuming the function is only called for delete, and normal character.
  if (action == REQ_DEL_CHAR || action == REQ_DEL_PREV) {
    // If the cursor lies at a position less than original field value length
    // after a successful delete action happens, then the successful action
    // deleted an user-input character and field value length is decreased.
    // Example: a b c d e _ _ _
    //          0 1 2 3 4 5 6 7
    // Field value length was 5. If cursor is at index 5 or larger after
    // deletion, no user input was deleted regardless of DELETE or BACKSPACE.
    // If cursor is at index 4 after char 'e' was deleted,
    // length should be decreased.
    if (curPos < curLen) {
      ptr->length = curLen - 1;
      set_field_userptr(current_field(form), static_cast<void*>(ptr));
    }
  } else {
    // Example: a b c d e _ _ _
    //          0 1 2 3 4 5 6 7
    // Field value length was 5.
    // If char 'f' was added at some position from 0 to 4, the cursor will
    // end up at index from 1 to 5, and length is increased by 1.
    // If char 'f' was added at some position >= 5, the cursor position will
    // be larger than 5 after addition, and the new length of the field value
    // is the index of the new cursor position.
    if (curPos <= curLen) {
      ptr->length = curLen + 1;
    } else {
      ptr->length = curPos;
    }
    set_field_userptr(current_field(form), static_cast<void*>(ptr));
  }
}

void CommandGui::quitGui() {
  // Free all ncurses objects
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
  delwin(exitWin);
  delwin(startWin);
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

std::string CommandGui::getFieldValueString(FIELD* field) {
  std::string value = std::string(field_buffer(field, 0));
  int len = static_cast<FieldAttrs*>(field_userptr(field))->length;
  return value.substr(0, len);
}

std::string CommandGui::getSourceRoot() {
  return getFieldValueString(basicFields[1]);
}

std::string CommandGui::getBuildDir() {
  return getFieldValueString(basicFields[3]);
}

std::string CommandGui::getOutputDir() {
  return getFieldValueString(basicFields[5]);
}

std::string CommandGui::getTestResultDir() {
  return getFieldValueString(basicFields[7]);
}

std::string CommandGui::getBuildCmd() {
  return getFieldValueString(basicFields[9]);
}

std::string CommandGui::getTestCmd() {
  return getFieldValueString(basicFields[11]);
}

std::string CommandGui::getWorkDir() {
  return getFieldValueString(advancedFields[1]);
}

std::vector<std::string> CommandGui::getTestResultFileExts() {
  return string::split(getFieldValueString(advancedFields[3]), ',');
}

std::vector<std::string> CommandGui::getTargetFileExts() {
  return string::split(getFieldValueString(advancedFields[5]), ',');
}

std::vector<std::string> CommandGui::getExcludePaths() {
  return string::split(getFieldValueString(advancedFields[7]), ',');
}

int CommandGui::getMutantLimit() {
  return std::stoul(
      string::trim(getFieldValueString(advancedFields[9])));
}

std::string CommandGui::getTestTimeLimit() {
  return  string::trim(getFieldValueString(advancedFields[11]));
}

std::string CommandGui::getKillAfter() {
  return string::trim(getFieldValueString(advancedFields[13]));
}

unsigned CommandGui::getSeed() {
  return std::stoul(
      string::trim(getFieldValueString(advancedFields[15])));
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
