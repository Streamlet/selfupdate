#include "../include/selfupdate/selfupdate.h"
#include "../utility/log.h"
#include "../utility/process_util.h"
#include "common.h"
#include <filesystem>

namespace selfupdate {

std::error_code Install(const PackageInfo &package_info,
                        std::filesystem::path installer_path /*= {}*/,
                        std::filesystem::path install_location /* = {}*/) {
  LOG_INFO("Installing:", package_info.package_name);

  std::error_code ec;
  std::filesystem::path cache_dir = std::filesystem::temp_directory_path(ec);
  if (ec) {
    LOG_ERROR("Get temp dir error. Error category:", ec.category().name(), ", code:", ec.value(),
              ", message:", ec.message());
    return ec;
  }
  cache_dir /= package_info.package_name;
  std::string package_file_name = package_info.package_name + PACKAGE_NAME_VERSION_SEP + package_info.package_version +
                                  FILE_NAME_EXT_SEP + package_info.package_format;
  std::filesystem::path package_file = cache_dir / package_file_name;
  if (!std::filesystem::exists(package_file, ec)) {
    LOG_ERROR("Package file missing:", package_file.u8string(), ", error category:", ec.category().name(),
              ", code:", ec.value(), ", message:", ec.message());
    return ec;
  }

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
  if (ec) {
    LOG_ERROR("Copy installer failed, from:", installer_path.u8string(), ", to:", copied_installer_path.u8string(),
              ", error category:", ec.category().name(), ", code:", ec.value(), ", message:", ec.message());
    return ec;
  }

  long pid = process_util::GetPid();
  TLOG_INFO(_T("Launching installer. Command line:"), copied_installer_path.c_str(), _T("--" INSTALLER_ARGUMENT_UPDATE),
            _T("--" INSTALLER_ARGUMENT_WAIT_PID), pid, _T("--" INSTALLER_ARGUMENT_SOURCE), package_file.c_str(),
            _T("--" INSTALLER_ARGUMENT_TARGET), install_location.c_str(), _T("--" INSTALLER_ARGUMENT_LAUNCH_FILE),
            exe_file.c_str());
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
  if (installer_pid == 0) {
    TLOG_ERROR(_T("Launch installer failed. Command line:"), copied_installer_path.c_str(),
               _T("--" INSTALLER_ARGUMENT_UPDATE), _T("--" INSTALLER_ARGUMENT_WAIT_PID), pid,
               _T("--" INSTALLER_ARGUMENT_SOURCE), package_file.c_str(), _T("--" INSTALLER_ARGUMENT_TARGET),
               install_location.c_str(), _T("--" INSTALLER_ARGUMENT_LAUNCH_FILE), exe_file.c_str());
    return make_selfupdate_error(SUE_RunInstallerError);
  }

  LOG_INFO("Launched installer");
  return {};
}

} // namespace selfupdate
