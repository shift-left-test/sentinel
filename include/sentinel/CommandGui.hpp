/*
 * Copyright (c) 2021 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef INCLUDE_SENTINEL_COMMANDGUI_HPP_
#define INCLUDE_SENTINEL_COMMANDGUI_HPP_

#include <ncurses.h>
#include <form.h>
#include <panel.h>
#include <cstddef>
#include <string>
#include <vector>
#include "sentinel/Command.hpp"
#include "sentinel/CommandRun.hpp"
#include "sentinel/ncstream/NcWindowStreambuf.hpp"


namespace sentinel {

/**
 * @brief FieldAttrs struct stores length of field value
 */
typedef struct {
  /**
   * brief field value length
   */
  int length;
} FieldAttrs;

/**
 * @brief sentinel commandline 'gui' subcommand class
 */
class CommandGui : public CommandRun {
 public:
  /**
   * @brief constructor
   */
  explicit CommandGui(args::Subparser& parser);

  int run() override;

 private:
  /**
   * @brief Initilize ncurses-based sentinel GUI
   */
  void initGui();

  /**
   * @brief Define what user can do with the GUI and
   *        what happen with respect to each action.
   */
  void handleUserInteraction();

  /**
   * @brief Deconstruct ncurses objects in GUI
   */
  void quitGui();

  /**
   * @brief Validate user-given configuration for mutation testing
   */
  bool validateConfiguration();

  /**
   * @brief run mutation testing with validated configuration
   */
  int startMutationTesting();

  /**
   * @brief Refresh output pad. Toggle cursor. Update scrollbar.
   */
  void refreshOutputWin();

  /**
   * @brief refresh scroll bar size and position based on current log status
   */
  void refreshOutputScrollbar();

  /**
   * @brief refresh help box with respect to currently selected configuration
   */
  void refreshHelpWin(FORM* form);

  /**
   * @brief Select the next field in the form.
   *
   * @param form of fields
   */
  void selectNextField(FORM* form);

  /**
   * @brief Select the previous field in the form.
   *
   * @param form of fields
   */
  void selectPrevField(FORM* form);

  /**
   * @brief Move to the next page in the form.
   *
   * @param form of fields
   */
  void moveNextPage(FORM* form);

  /**
   * @brief Move to the previous page in the form.
   *
   * @param form of fields
   */
  void movePrevPage(FORM* form);

  /**
   * @brief update field value length
   *
   * @param form target form
   * @param action REQ_DEL_CHAR or REQ_DEL_PREV or any non-special character.
   */
  void updateFieldValueLength(FORM* form, int action);

  /**
   * @brief return the idx-th field value of form as a string
   *
   * @param field target field
   * @return field value string
   */
  std::string getFieldValueString(FIELD* field);

 protected:
  /**
   * @brief set signal hander
   */
  void setSignalHandler() override;

  /**
   * @brief return source root path
   * @return source root path
   */
  std::string getSourceRoot() override;

  /**
   * @brief build directory
   * @return build directory
   */
  std::string getBuildDir() override;

  /**
   * @brief get work directory
   * @return work directory
   */
  std::string getWorkDir() override;

  /**
   * @brief get output directory
   * @return output directory
   */
  std::string getOutputDir() override;

  /**
   * @brief get test result directory
   * @return test result directory
   */
  std::string getTestResultDir() override;

  /**
   * @brief get build command
   * @return build command
   */
  std::string getBuildCmd() override;

  /**
   * @brief get test command
   * @return test command
   */
  std::string getTestCmd() override;

  /**
   * @brief get mutant generator type
   * @return mutant generator type
   */
  std::string getGenerator() override;

  /**
   * @brief get test result file extensions
   * @return test result file extension list
   */
  std::vector<std::string> getTestResultFileExts() override;

  /**
   * @brief get file extensions as target for mutation
   * @return target file extension list
   */
  std::vector<std::string> getTargetFileExts() override;

  /**
   * @brief get excluded paths
   * @return list of excluded paths
   */
  std::vector<std::string> getExcludePaths() override;

  /**
   * @brief get coverage files
   * @return list of coverage files
   */
  std::vector<std::string> getCoverageFiles() override;

  /**
   * @brief get mutation scope
   * @return mutation scope
   */
  std::string getScope() override;

  /**
   * @brief get maximum number of mutant to be generated
   * @return mutant limit
   */
  size_t getMutantLimit() override;

  /**
   * @brief get test timeout
   * @return test timeout
   */
  std::string getTestTimeLimit() override;

  /**
   * @brief get duration after which SIGKILL is sent to timeout-ed test process
   * @return time after which test process is killed forcefully
   */
  std::string getKillAfter() override;

  /**
   * @brief get random seed
   * @return random seed
   */
  unsigned getSeed() override;

  /**
   * @brief get verbose status
   * @return verbose status
   */
  bool getVerbose() override;

 private:
  WindowInfo titleWinInfo;
  WindowInfo outputPadInfo;
  WindowInfo outputWinInfo;
  WindowInfo configWinInfo;
  WindowInfo outputScrollInfo;
  WindowInfo helpWinInfo;
  WindowInfo helpPadInfo;
  WindowInfo advancedWinInfo;
  WindowInfo exitWinInfo;
  WindowInfo startWinInfo;

  WINDOW* titleWin;
  WINDOW* outputPad;
  WINDOW* outputWin;
  WINDOW* configWin;
  WINDOW* outputScroll;
  WINDOW* helpWin;
  WINDOW* helpPad;
  WINDOW* advancedWin;
  WINDOW* exitWin;
  WINDOW* startWin;
  PANEL* titlePanel;
  PANEL* advancedPanel;
  PANEL* exitPanel;
  PANEL* startPanel;

  std::vector<FIELD*> basicFields;
  std::vector<FIELD*> advancedFields;
  FORM* basicForm;
  FORM* advancedForm;

  int outputPadCurrSize;
  int outputPadCurrPos;
  int outputPadScrollBufferSize;
  int currConfigPage;
  int numBasicConfigPages;
  int configScrollbarSize;
  std::vector<std::vector<std::string>> basicOptions;
  std::vector<std::vector<std::string>> advancedOptions;
};

}  // namespace sentinel

#endif  // INCLUDE_SENTINEL_COMMANDGUI_HPP_
