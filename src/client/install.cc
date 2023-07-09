#include "../include/selfupdate/selfupdate.h"
#include "common.h"
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/process.hpp>
#include <boost/process/environment.hpp>
#include <boost/scope_exit.hpp>
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

  std::filesystem::path exe_path = boost::dll::program_location();
  std::filesystem::path exe_dir = exe_path.parent_path();
  std::filesystem::path exe_file = exe_path.filename();

  if (installer_path.empty())
    installer_path = boost::dll::program_location();
  if (install_location.empty())
    install_location = exe_dir;

  std::filesystem::path copied_installer_path = package_file.parent_path() / installer_path.filename();
  std::filesystem::copy_file(installer_path, copied_installer_path, std::filesystem::copy_options::overwrite_existing,
                             ec);
  if (ec)
    return ec;

  int pid = boost::this_process::get_id();
  tstringstream ss;
  ss << copied_installer_path.native();
  ss << _T(" --" INSTALLER_ARGUMENT_UPDATE);
  ss << _T(" --" INSTALLER_ARGUMENT_WAIT_PID) << _T("=") << pid;
  ss << _T(" --" INSTALLER_ARGUMENT_SOURCE) << _T("=\"") << package_file.native() << _T("\"");
  ss << _T(" --" INSTALLER_ARGUMENT_TARGET) << _T("=\"") << install_location.native() << _T("\"");
  ss << _T(" --" INSTALLER_ARGUMENT_LAUNCH_FILE) << _T("=\"") << exe_file.native() << _T("\"");
  tstring cmd = ss.str();
  boost::process::spawn(cmd, boost::process::start_dir(copied_installer_path.parent_path().native()));
  return {};
}

} // namespace selfupdate
