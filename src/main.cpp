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

#include <iostream>
#include <args/args.hxx>


args::Group arguments("arguments");
args::HelpFlag h(arguments, "help",
  "Display this help menu. \n"
  "Use 'sentinal COMMAND --help' to see help for each command.",
  {'h', "help"});

void populateCommand(args::Subparser &parser);  // NOLINT
void mutateCommand(args::Subparser &parser);  // NOLINT
void evaluateCommand(args::Subparser &parser);  // NOLINT
void reportCommand(args::Subparser &parser);  // NOLINT

int main(int argc, char ** argv) {
  args::ArgumentParser parser("Mutation Test");

  args::Group commands(parser, "commands");
  args::Command populate(commands, "populate",
    "Identify mutable test targets and application methods in'git' "
    "and print a list",
    &populateCommand);
  args::Command mutate(commands, "mutate",
    "Apply the selected 'mutable' to the source",
    &mutateCommand);
  args::Command evaluate(commands, "evaluate",
    "Compare the test result with mutable applied and the test result "
    "not applied",
    &evaluateCommand);
  args::Command report(commands, "report",
    "Create a mutation test report based on the'evaluate' result "
    "and source code",
    &reportCommand);

  args::GlobalOptions globals(parser, arguments);

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help& e) {
    std::cout << parser;
  } catch (args::Error& e) {
    std::cerr << e.what() << std::endl << parser;
    return 1;
  }

  return 0;
}
