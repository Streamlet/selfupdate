#include "../include/selfupdate/installer.h"
#include "common.h"
#include <boost/algorithm/string.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <filesystem>
#include <loki/ScopeGuard.h>
#include <zlibwrap/zlibwrap.h>

namespace selfupdate {

bool IsInstallMode(int argc, const char *argv[], InstallContext &install_context) {
  boost::program_options::options_description desc;
  desc.add_options()(INSTALLER_ARGUMENT_UPDATE,
                     "")(INSTALLER_ARGUMENT_WAIT_PID, boost::program_options::value<int>(),
                         "")(INSTALLER_ARGUMENT_SOURCE, boost::program_options::value<std::string>(),
                             "")(INSTALLER_ARGUMENT_TARGET, boost::program_options::value<std::string>(),
                                 "")(INSTALLER_ARGUMENT_LAUNCH_FILE, boost::program_options::value<std::string>(), "");
  boost::program_options::variables_map vm;
  try {
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  } catch (boost::program_options::unknown_option e) {
    return false;
  } catch (boost::program_options::invalid_option_value e) {
    return false;
  }
  boost::program_options::notify(vm);

  if (!vm.count(INSTALLER_ARGUMENT_UPDATE) || !vm.count(INSTALLER_ARGUMENT_WAIT_PID) ||
      !vm.count(INSTALLER_ARGUMENT_SOURCE) || !vm.count(INSTALLER_ARGUMENT_TARGET) ||
      !vm.count(INSTALLER_ARGUMENT_LAUNCH_FILE)) {
    return false;
  }

  install_context.wait_pid = vm[INSTALLER_ARGUMENT_WAIT_PID].as<int>();
  install_context.source = vm[INSTALLER_ARGUMENT_SOURCE].as<std::string>();
  install_context.target = vm[INSTALLER_ARGUMENT_TARGET].as<std::string>();
  install_context.launch_file = vm[INSTALLER_ARGUMENT_LAUNCH_FILE].as<std::string>();

  auto is_quote = [](char ch) {
    return ch == '"';
  };
  boost::algorithm::trim_if(install_context.source, is_quote);
  boost::algorithm::trim_if(install_context.target, is_quote);
  boost::algorithm::trim_if(install_context.launch_file, is_quote);

  return true;
}

namespace {

const char *INSTALL_LOCATION_OLD_SUFFIX = ".old";
const char *INSTALL_LOCATION_NEW_SUFFIX = ".new";

std::error_code
InstallZipPackage(std::filesystem::path package_file, std::filesystem::path install_location, std::string launch_file) {
  std::filesystem::path install_location_old = install_location;
  install_location_old += INSTALL_LOCATION_OLD_SUFFIX;
  std::filesystem::remove_all(install_location_old);
  std::filesystem::path install_location_new = install_location;
  install_location_new += INSTALL_LOCATION_NEW_SUFFIX;
  std::filesystem::remove_all(install_location_new);

  if (!zlibwrap::ZipExtract(package_file.string().c_str(), install_location_new.string().c_str()))
    return make_selfupdate_error(SUE_PackageExtractError);

  std::error_code ec;
  if (std::filesystem::exists(install_location)) {
    std::filesystem::rename(install_location, install_location_old, ec);
    if (ec)
      return ec;
  }
  if (std::filesystem::exists(install_location))
    return make_selfupdate_error(SUE_MoveFileError);
  std::filesystem::rename(install_location_new, install_location, ec);
  if (ec)
    return ec;
  if (!std::filesystem::exists(install_location))
    return make_selfupdate_error(SUE_MoveFileError);

  if (std::filesystem::exists(install_location_old))
    std::filesystem::remove_all(install_location_old);

  std::filesystem::path launch_path = install_location / launch_file;
  std::filesystem::permissions(launch_path,
                               std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec |
                                   std::filesystem::perms::others_exec,
                               std::filesystem::perm_options::add);
  boost::process::spawn(launch_path.string());

  return {};
}

} // namespace

const int INSTALL_WAIT_FOR_MAIN_PROCESS = 10;

std::error_code DoInstall(const InstallContext &install_context) {
  auto exe_path = boost ::dll::program_location();
  if (exe_path.string().find(install_context.target) == 0)
    return make_selfupdate_error(SUE_RunInstallerPositionError);

  boost::process::child main_process(install_context.wait_pid);
  if (main_process.valid() && main_process.running()) {
    if (!main_process.wait_for(std::chrono::seconds(INSTALL_WAIT_FOR_MAIN_PROCESS))) {
      main_process.terminate();
    }
  }

  std::filesystem::path package_file = install_context.source;
  std::filesystem::path install_location = install_context.target;

  std::string package_format = package_file.extension().string();
  if (package_format == std::string(FILE_NAME_EXT_SEP) + PACKAGEINFO_PACKAGE_FORMAT_ZIP) {
    return InstallZipPackage(std::move(package_file), std::move(install_location),
                             std::move(install_context.launch_file));
  } else {
    return make_selfupdate_error(SUE_UnsupportedPackageFormat);
  }
}

} // namespace selfupdate
