#pragma once

#include <system_error>

namespace selfupdate {

struct InstallContext;

const InstallContext *IsInstallMode(int argc, const char *argv[]);
const InstallContext *IsInstallMode(int argc, const wchar_t *argv[]);
#ifdef _WIN32
const InstallContext *IsInstallMode(const char *cmdline);
const InstallContext *IsInstallMode(const wchar_t *cmdline);
#endif

std::error_code DoInstall(const InstallContext *install_context);

} // namespace selfupdate
