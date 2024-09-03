#include "../common.h"
#include <selfupdate/updater.h>
#include <xl/file>
#include <xl/log>
#include <xl/native_string>
#include <xl/process>

namespace selfupdate {

bool Install(const PackageInfo &package_info, const TCHAR *installer_path, const TCHAR *install_location) {
  XL_LOG_INFO("Installing: ", package_info.package_name);

  xl::native_string cache_dir = xl::fs::tmp_dir();
  if (cache_dir.empty()) {
    XL_LOG_ERROR("Get temp dir error.");
    return false;
  }
  std::string package_file_name = package_info.package_name + PACKAGE_NAME_VERSION_SEP + package_info.package_version +
                                  FILE_NAME_EXT_SEP + package_info.package_format;
  xl::native_string package_file = xl::path::join(cache_dir, xl::encoding::utf8_to_native(package_info.package_name),
                                                  xl::encoding::utf8_to_native(package_file_name));
  if (!xl::fs::exists(package_file.c_str())) {
    XL_LOG_ERROR("Package file missing: ", package_file);
    return false;
  }

  xl::native_string exe_path = xl::process::executable_path();
  xl::native_string exe_dir = xl::path::dirname(exe_path.c_str());
  xl::native_string exe_file = xl::path::filename(exe_path.c_str());

  if (installer_path == nullptr) {
    installer_path = exe_path.c_str();
  }
  if (install_location == nullptr) {
    install_location = exe_dir.c_str();
  }

  xl::native_string copied_installer_path =
      xl::path::join(xl::path::dirname(package_file.c_str()), xl::path::filename(installer_path));
  if (!xl::fs::copy(installer_path, copied_installer_path.c_str())) {
    XL_LOG_ERROR("Copy installer failed, from: ", installer_path, ", to: ", copied_installer_path.c_str());
    return false;
  }

#ifndef _WIN32
  chmod(copied_installer_path.c_str(), S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP | S_IXUSR | S_IXGRP | S_IXOTH);
#endif

  long pid = xl::process::pid();
  XL_LOG_INFO(_T("Launching installer. Command line: "), copied_installer_path.c_str(),
              _T(" --" INSTALLER_ARGUMENT_UPDATE " "), _T(" --" INSTALLER_ARGUMENT_WAIT_PID " "), pid,
              _T(" --" INSTALLER_ARGUMENT_FORCE_UPDATE " "), package_info.force_update ? _T("1") : _T("0"),
              _T(" --" INSTALLER_ARGUMENT_SOURCE " "), package_file.c_str(), _T(" --" INSTALLER_ARGUMENT_TARGET " "),
              install_location, _T(" --" INSTALLER_ARGUMENT_LAUNCH_FILE " "), exe_file.c_str());
  long installer_pid = xl::process::start(copied_installer_path,
                                          {
                                              _T("--" INSTALLER_ARGUMENT_UPDATE),
                                              _T("--" INSTALLER_ARGUMENT_WAIT_PID),
                                              xl::to_native_string(pid),
                                              _T("--" INSTALLER_ARGUMENT_FORCE_UPDATE),
                                              package_info.force_update ? _T("1") : _T("0"),
                                              _T("--" INSTALLER_ARGUMENT_SOURCE),
                                              package_file,
                                              _T("--" INSTALLER_ARGUMENT_TARGET),
                                              install_location,
                                              _T("--" INSTALLER_ARGUMENT_LAUNCH_FILE),
                                              exe_file,
                                          },
                                          xl::path::dirname(copied_installer_path.c_str()));
  if (installer_pid == 0) {
    XL_LOG_ERROR(_T("Launch installer failed. Command line: "), copied_installer_path.c_str(),
                 _T(" --" INSTALLER_ARGUMENT_UPDATE " "), _T("--" INSTALLER_ARGUMENT_WAIT_PID " "), pid,
                 _T(" --" INSTALLER_ARGUMENT_FORCE_UPDATE " "), package_info.force_update ? _T("1") : _T("0"),
                 _T(" --" INSTALLER_ARGUMENT_SOURCE " "), package_file.c_str(), _T(" --" INSTALLER_ARGUMENT_TARGET " "),
                 install_location, _T(" --" INSTALLER_ARGUMENT_LAUNCH_FILE " "), exe_file.c_str());
    return false;
  }

  XL_LOG_INFO("Launched installer");
  return true;
}

} // namespace selfupdate
