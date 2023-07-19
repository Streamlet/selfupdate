#include "../include/selfupdate/selfupdate.h"
#include "../utility/process_util.h"
#include "common.h"
#include <filesystem>
#include <sstream>

namespace selfupdate {

std::error_code Install(const PackageInfo &package_info,
                        std::filesystem::path installer_path /*= {}*/,
                        std::filesystem::path install_location /* = {}*/) {
  std::error_code ec;
  std::filesystem::path cache_dir = std::filesystem::temp_directory_path(ec);
  if (ec)
    return ec;
  cache_dir /= package_info.package_name;
  std::string package_file_name = package_info.package_name + PACKAGE_NAME_VERSION_SEP + package_info.package_version +
                                  FILE_NAME_EXT_SEP + package_info.package_format;
  std::filesystem::path package_file = cache_dir / package_file_name;
  if (!std::filesystem::exists(package_file, ec))
    return ec;

  std::filesystem::path exe_path = process_util::GetExecutablePath();
  std::filesystem::path exe_dir = exe_path.parent_path();
  std::filesystem::path exe_file = exe_path.filename();

  if (installer_path.empty())
    installer_path = exe_path;
  if (install_location.empty())
    install_location = exe_dir;

  std::filesystem::path copied_installer_path = package_file.parent_path() / installer_path.filename();
  std::filesystem::copy_file(installer_path, copied_installer_path, std::filesystem::copy_options::overwrite_existing,
                             ec);
  if (ec)
    return ec;

  long pid = process_util::GetPid();
  long installer_pid = process_util::StartProcess(copied_installer_path.native(),
                                                  {
                                                      _T("--" INSTALLER_ARGUMENT_UPDATE),
                                                      _T("--" INSTALLER_ARGUMENT_WAIT_PID),
                                                      to_native_string(pid),
                                                      _T("--" INSTALLER_ARGUMENT_SOURCE),
                                                      package_file.native(),
                                                      _T("--" INSTALLER_ARGUMENT_TARGET),
                                                      install_location.native(),
                                                      _T("--" INSTALLER_ARGUMENT_LAUNCH_FILE),
                                                      exe_file.native(),
                                                  },
                                                  copied_installer_path.parent_path().native());
  if (installer_pid == 0)
    return make_selfupdate_error(SUE_RunInstallerError);

  return {};
}

} // namespace selfupdate
