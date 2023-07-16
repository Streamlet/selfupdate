#include "../include/selfupdate/installer.h"
#include "../utility/cmdline_options.h"
#include "../utility/native_string.h"
#include "common.h"

namespace selfupdate {

namespace {
bool IsNewVersionFirstLaunched(const cmdline_options::ParsedOption &options) {
  return options.has(_T(INSTALLER_ARGUMENT_NEW_VERSION));
}

} // namespace

bool IsNewVersionFirstLaunched(int argc, const TCHAR *argv[]) {
  return IsNewVersionFirstLaunched(cmdline_options::parse(argc, argv));
}

#ifdef _WIN32
bool IsNewVersionFirstLaunched(const TCHAR *command_line) {
  return IsNewVersionFirstLaunched(cmdline_options::parse(command_line));
}
#endif

} // namespace selfupdate