#include "../include/selfupdate/installer.h"
#include "../utility/cmdline_options.h"
#include "common.h"

namespace selfupdate {

namespace {

bool IsNewVersionFirstLaunched(const std::map<std::string, std::string> &cmdline_options) {
  if (cmdline_options.find(INSTALLER_ARGUMENT_NEW_VERSION) == cmdline_options.end()) {
    return false;
  }
  return true;
}

bool IsNewVersionFirstLaunched(const std::map<std::wstring, std::wstring> &cmdline_options) {
  if (cmdline_options.find(_L(INSTALLER_ARGUMENT_NEW_VERSION)) == cmdline_options.end()) {
    return false;
  }
  return true;
}

} // namespace

bool IsNewVersionFirstLaunched(int argc, const char *argv[]) {
  return IsNewVersionFirstLaunched(std::move(cmdline_options::parse(argc, argv)));
}

bool IsNewVersionFirstLaunched(int argc, const wchar_t *argv[]) {
  return IsNewVersionFirstLaunched(std::move(cmdline_options::parse(argc, argv)));
}

#ifdef _WIN32
bool IsNewVersionFirstLaunched(const char *command_line) {
  return IsNewVersionFirstLaunched(std::move(cmdline_options::parse(command_line)));
}
bool IsNewVersionFirstLaunched(const wchar_t *command_line) {
  return IsNewVersionFirstLaunched(std::move(cmdline_options::parse(command_line)));
}
#endif

} // namespace selfupdate