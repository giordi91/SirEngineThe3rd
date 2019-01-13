#pragma once
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

// regex used to pars plugin arguments
static const std::regex
    ARGS_REGEX("(--[a-zA-Z]+)|(\\.*\\.*[_:/a-zA-Z]+\\.[a-zA-Z]+)");

struct SplitArgs {
  int argc = -1;
  std::unique_ptr<char *[]> argv;
  std::unique_ptr<std::vector<std::string>> storage;
};

// This function is in charge to grab a string and split it up in
// a way that resamble the input argc argv from the main function
// the reason for that is mostly that we are dealing with a library
// that nicely parses argc/argv, but can't be initialized from a string.
// when we forward args of plugins to a plugin, it is in string format and
// we need to process it
SplitArgs splitArgs(const std::string &args) {

  // build the regex iterator
  std::sregex_iterator it(args.begin(), args.end(), ARGS_REGEX);
  std::sregex_iterator it_end;

  SplitArgs sp;
  // allocate the needed vector that will hold the strings*
  sp.storage = std::make_unique<std::vector<std::string>>();
  while (it != it_end) {
    sp.storage->push_back((*it).str());
    ++it;
  }

  sp.argc = sp.storage->size();
  sp.argv = std::make_unique<char *[]>(sp.argc + 1);

  // now that we splitted the strings we are going to extract the
  // pointers and put them in the argv array
  for (int i = 0; i < sp.argc; ++i) {
    // skipping the first argument which is the  executable path
    sp.argv[i + 1] = (char *)(*sp.storage)[i].c_str();
  }
  // setting a dummy executable path
  sp.argv[0] = (char *)"dummy.exe";
  sp.argc += 1;
  return sp;
}
