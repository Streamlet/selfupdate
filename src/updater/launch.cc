#include "../common.h"
#include <selfupdate/updater.h>
#include <xl/cmdline_options>
#include <xl/native_string>

namespace selfupdate {

namespace {

bool IsNewVersionFirstLaunched(const xl::cmdline_options::parsed_options &options) {
  return options.has(_T(INSTALLER_ARGUMENT_NEW_VERSION));
}

bool IsForceUpdated(const xl::cmdline_options::parsed_options &options) {
  return options.get_as<bool>(_T(INSTALLER_ARGUMENT_FORCE_UPDATE));
}

} // namespace

bool IsNewVersionFirstLaunched(int argc, const TCHAR *argv[]) {
  return IsNewVersionFirstLaunched(xl::cmdline_options::parse(argc, argv));
}

bool IsForceUpdated(int argc, const TCHAR *argv[]) {
  return IsForceUpdated(xl::cmdline_options::parse(argc, argv));
}

#ifdef _WIN32

bool IsNewVersionFirstLaunched(const TCHAR *command_line) {
  return IsNewVersionFirstLaunched(xl::cmdline_options::parse(command_line));
}

bool IsForceUpdated(const TCHAR *command_line) {
  return IsForceUpdated(xl::cmdline_options::parse(command_line));
}

#endif

} // namespace selfupdate