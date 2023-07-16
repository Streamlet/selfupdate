
#include "cmdline_options.h"
#include "native_string.h"
#include <cstring>
#include <cwchar>
#include <loki/ScopeGuard.h>
#ifdef _WIN32
#include "encoding.h"
#include <shellapi.h>
#include <windows.h>
#endif

namespace cmdline_options {

namespace {

ParsedOption<TCHAR> parse_native(int argc, const TCHAR *argv[]) {
  std::map<native_string, native_string> result;
  for (int i = 1; i < argc; ++i) {
    native_string arg = argv[i];
    if (arg.length() >= 2 && arg[0] == _T('-') && arg[1] == _T('-')) {
      auto equal_pos = arg.find_first_of(_T('='), 2);
      if (equal_pos != native_string::npos) {
        native_string k = arg.substr(2, equal_pos - 2);
        native_string v = arg.substr(equal_pos + 1);
        result.insert(std::make_pair(k, v));
      } else {
        native_string k = arg.substr(2);
        result.insert(std::make_pair(k, native_string()));
      }
    } else if (arg.length() >= 1 && arg[0] == _T('-')) {
      native_string k = arg.substr(1);
      if (i + 1 < argc && argv[i + 1][0] != _T('-')) {
        result.insert(std::make_pair(k, argv[i + 1]));
        ++i;
      } else {
        result.insert(std::make_pair(k, native_string()));
      }
    } else {
      result.insert(std::make_pair(arg, native_string()));
    }
  }
  return {std::move(result)};
}

} // namespace

#ifdef _WIN32

ParsedOption<TCHAR> parse(int argc, const TCHAR *argv[]) {
  return parse_native(argc, argv);
}

#ifdef _UNICODE
ParsedOption<wchar_t> parse(const wchar_t *cmdline) {
  int argc = 0;
  LPWSTR *argv = ::CommandLineToArgvW(cmdline, &argc);
  if (argv == NULL)
    return {};
  LOKI_ON_BLOCK_EXIT(::LocalFree, argv);
  return parse_native(argc, (LPCWSTR *)argv);
}

#else

ParsedOption<char> parse(const char *cmdline) {
  std::wstring wstr = encoding::ANSIToUCS2(cmdline);
  int argc = 0;
  LPWSTR *argv = ::CommandLineToArgvW(cmdline, &argc);
  if (argv == NULL)
    return {};
  LOKI_ON_BLOCK_EXIT(::LocalFree, argv);
  std::vector<std::string> argv_str;
  for (int i = 0; i < argc; ++i) {
    argv_str.push_back(encoding::UCS2ToANSI(argv[i]));
  }
  std::vector<const char *> argv_cstr;
  for (int i = 0; i < argc; ++i) {
    argv_cstr.push_back(argv_str[i].c_str());
  }
  return parse_native(argc, &argv_cstr[0]);
}

#endif

#else

ParsedOption<char> parse(int argc, const char *argv[]) {
  return parse_native(argc, argv);
}

#endif

} // namespace cmdline_options
