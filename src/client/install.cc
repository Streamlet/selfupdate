#include "../include/selfupdate/selfupdate.h"
#include "common.h"
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/process.hpp>
#include <boost/process/environment.hpp>
#include <filesystem>
#include <loki/ScopeGuard.h>
#include <sstream>

namespace selfupdate {

std::error_code Install(const PackageInfo &package_info,
                        std::filesystem::path installer_path,
                        std::filesystem::path install_location /* = {}*/,
                        std::filesystem::path launch_file /* = {}*/) {
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

  auto exe_path = std::filesystem::path(boost ::dll::program_location().string());
  auto exe_dir = exe_path.parent_path();
  auto exe_file = exe_path.filename();

  if (install_location.empty())
    install_location = exe_dir.c_str();
  if (launch_file.empty())
    launch_file = exe_file.c_str();

  std::filesystem::path copied_installer_path = package_file.parent_path() / installer_path.filename();
  std::filesystem::copy_file(installer_path, copied_installer_path, std::filesystem::copy_options::overwrite_existing,
                             ec);
  if (ec)
    return ec;

  int pid = boost::this_process::get_id();
  const std::string program_option_prefix = "--";
  std::stringstream ss;
  ss << copied_installer_path.string();
  ss << " --\"" << INSTALLER_ARGUMENT_UPDATE << "\"";
  ss << " --" << INSTALLER_ARGUMENT_WAIT_PID << "=" << pid;
  ss << " --" << INSTALLER_ARGUMENT_SOURCE << "=\"" << package_file.string() << "\"";
  ss << " --" << INSTALLER_ARGUMENT_TARGET << "=\"" << install_location.string() << "\"";
  ss << " --" << INSTALLER_ARGUMENT_LAUNCH_FILE << "=\"" << launch_file.string() << "\"";
  std::string cmd = ss.str();
#ifdef _DEBUG
  printf("Launch updater: %s\n", cmd.c_str());
#endif
  boost::process::spawn(cmd);
  return {};
}

} // namespace selfupdate
