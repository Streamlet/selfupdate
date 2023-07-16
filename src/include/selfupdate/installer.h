#pragma once

#include <system_error>

#ifdef _WIN32
#include <tchar.h>
#endif

namespace selfupdate {

struct InstallContext;

#ifdef _WIN32
const InstallContext *IsInstallMode(int argc, const TCHAR *argv[]);
const InstallContext *IsInstallMode(const TCHAR *cmdline);
#else
const InstallContext *IsInstallMode(int argc, const char *argv[]);
#endif

std::error_code DoInstall(const InstallContext *install_context);

} // namespace selfupdate
