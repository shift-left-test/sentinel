# Copyright (c) 2026 LG Electronics Inc.
# SPDX-License-Identifier: MIT

# --- screen ---
add_library(ftxui-screen STATIC
  src/ftxui/screen/box.cpp
  src/ftxui/screen/color.cpp
  src/ftxui/screen/color_info.cpp
  src/ftxui/screen/image.cpp
  src/ftxui/screen/screen.cpp
  src/ftxui/screen/string.cpp
  src/ftxui/screen/terminal.cpp
)
target_include_directories(ftxui-screen SYSTEM PUBLIC include)
target_include_directories(ftxui-screen PRIVATE src)
target_compile_features(ftxui-screen PUBLIC cxx_std_17)

# --- dom ---
add_library(ftxui-dom STATIC
  src/ftxui/dom/automerge.cpp
  src/ftxui/dom/blink.cpp
  src/ftxui/dom/bold.cpp
  src/ftxui/dom/border.cpp
  src/ftxui/dom/box_helper.cpp
  src/ftxui/dom/canvas.cpp
  src/ftxui/dom/clear_under.cpp
  src/ftxui/dom/color.cpp
  src/ftxui/dom/composite_decorator.cpp
  src/ftxui/dom/dbox.cpp
  src/ftxui/dom/dim.cpp
  src/ftxui/dom/flex.cpp
  src/ftxui/dom/flexbox.cpp
  src/ftxui/dom/flexbox_config.cpp
  src/ftxui/dom/flexbox_helper.cpp
  src/ftxui/dom/focus.cpp
  src/ftxui/dom/frame.cpp
  src/ftxui/dom/gauge.cpp
  src/ftxui/dom/graph.cpp
  src/ftxui/dom/gridbox.cpp
  src/ftxui/dom/hbox.cpp
  src/ftxui/dom/hyperlink.cpp
  src/ftxui/dom/inverted.cpp
  src/ftxui/dom/italic.cpp
  src/ftxui/dom/linear_gradient.cpp
  src/ftxui/dom/node.cpp
  src/ftxui/dom/node_decorator.cpp
  src/ftxui/dom/paragraph.cpp
  src/ftxui/dom/reflect.cpp
  src/ftxui/dom/scroll_indicator.cpp
  src/ftxui/dom/selection.cpp
  src/ftxui/dom/selection_style.cpp
  src/ftxui/dom/separator.cpp
  src/ftxui/dom/size.cpp
  src/ftxui/dom/spinner.cpp
  src/ftxui/dom/strikethrough.cpp
  src/ftxui/dom/table.cpp
  src/ftxui/dom/text.cpp
  src/ftxui/dom/underlined.cpp
  src/ftxui/dom/underlined_double.cpp
  src/ftxui/dom/util.cpp
  src/ftxui/dom/vbox.cpp
)
target_include_directories(ftxui-dom SYSTEM PUBLIC include)
target_include_directories(ftxui-dom PRIVATE src)
target_link_libraries(ftxui-dom PUBLIC ftxui-screen)
