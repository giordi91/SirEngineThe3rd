#pragma once
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

// regex used to pars plugin arguments
static const std::regex ARGS_REGEX(
    "(--*[a-zA-Z]+)|(\\.*\\.*[_:/a-zA-Z0-9]+\\.[a-zA-Z]+)|([/"
    "_a-zA-Z0-9]+)|(\".+\")");

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
inline SplitArgs splitArgs(const std::string &args) {
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

  sp.argc = static_cast<int>(sp.storage->size());
  sp.argv = std::make_unique<char *[]>(sp.argc + 1);

  // now that we splitted the strings we are going to extract the
  // pointers and put them in the argv array
  for (int i = 0; i < sp.argc; ++i) {
    // skipping the first argument which is the  executable path
    sp.argv[i + 1] = (char *)(*sp.storage)[i].c_str();
  }
  // setting a dummy executable path, so we can reuse the library for
  // parsing console arguments
  sp.argv[0] = (char *)"dummy.exe";
  sp.argc += 1;
  return sp;
}

// This function is used to get a string with extra compiler args
// the user pass in and to get it read to be used
inline void splitCompilerArgs(
    const std::string &args, std::vector<std::wstring> &splitCompilerArgsList,
    std::vector<wchar_t *> &splitCompilerArgsListPointers) {
  SplitArgs intermediateArgs = splitArgs(args);

  // removing the dummy
  int realArgCount = intermediateArgs.argc - 1;
  // Now we have a vector of split arguments we need to  convert them to
  // wstrings and patch the pointers
  int finalArgCount = realArgCount / 2;


  for (int i = 0; i < realArgCount; i += 2) {
    // highly inefficient I know but don't wont to get bogged
    // into the shader compiler forever;
    std::string currArg = (*intermediateArgs.storage)[i];
    currArg += " ";
    currArg += (*intermediateArgs.storage)[i + 1];
    splitCompilerArgsList.emplace_back(
        std::wstring(currArg.begin(), currArg.end()));
  }
  // extracting the pointers
  for (int i = 0; i < finalArgCount; ++i) {
    splitCompilerArgsListPointers.push_back( 
        (wchar_t *)splitCompilerArgsList[i].c_str());
  }
}
