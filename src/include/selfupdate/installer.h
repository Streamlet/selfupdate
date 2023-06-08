#pragma once

#include <string>
#include <system_error>

namespace selfupdate {

struct InstallContext {
  int wait_pid = 0;
  std::string source;
  std::string target;
  std::string launch_file;
};

bool IsInstallMode(int argc, const char *argv[], InstallContext &install_context);
std::error_code DoInstall(const InstallContext &install_context);

} // namespace selfupdate
