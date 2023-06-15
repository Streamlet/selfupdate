#include "../include/selfupdate/installer.h"
#include "common.h"
#include <boost/algorithm/string.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <filesystem>
#include <loki/ScopeGuard.h>
#include <sstream>
#include <stdio.h>
#include <zlibwrap/zlibwrap.h>
#ifdef _WIN32
#include <Windows.h>
#endif

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
const int INSTALL_WAIT_FOR_MAIN_PROCESS = 10;

std::error_code InstallZipPackage(const std::filesystem::path &package_file,
                                  const std::filesystem::path &install_location) {
  std::filesystem::path install_location_old = install_location;
  install_location_old += INSTALL_LOCATION_OLD_SUFFIX;
  std::filesystem::remove_all(install_location_old);
  std::filesystem::path install_location_new = install_location;
  install_location_new += INSTALL_LOCATION_NEW_SUFFIX;
  std::filesystem::remove_all(install_location_new);

  if (!zlibwrap::ZipExtract(package_file.string().c_str(), install_location_new.string().c_str()))
    return make_selfupdate_error(SUE_PackageExtractError);

  std::error_code ec;
  for (int i = 0; i < INSTALL_WAIT_FOR_MAIN_PROCESS && std::filesystem::exists(install_location); ++i) {
    std::filesystem::rename(install_location, install_location_old, ec);
    if (ec) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  if (std::filesystem::exists(install_location))
    return make_selfupdate_error(SUE_MoveFileError);
  std::filesystem::rename(install_location_new, install_location, ec);
  if (ec)
    return ec;
  if (!std::filesystem::exists(install_location))
    return make_selfupdate_error(SUE_MoveFileError);

  if (std::filesystem::exists(install_location_old)) {
    for (const std::filesystem::directory_entry &dir_entry :
         std::filesystem::recursive_directory_iterator(install_location_old)) {
      std::filesystem::path file_in_new_path =
          install_location / dir_entry.path().lexically_relative(install_location_old);
      if (!std::filesystem::exists(file_in_new_path))
        std::filesystem::rename(dir_entry.path(), file_in_new_path);
    }
    std::filesystem::remove_all(install_location_old);
  }

  return {};
}

} // namespace

std::error_code DoInstall(const InstallContext &install_context) {
  auto exe_path = std::filesystem::path(boost ::dll::program_location().string());
  if (exe_path.string().find(install_context.target) == 0)
    return make_selfupdate_error(SUE_RunInstallerPositionError);

  try {
    boost::process::child main_process(static_cast<boost::process::pid_t>(install_context.wait_pid));
    if (main_process.valid() && main_process.running()) {
      if (!main_process.wait_for(std::chrono::seconds(INSTALL_WAIT_FOR_MAIN_PROCESS))) {
        main_process.terminate();
      }
    }
  } catch (boost::process::process_error e) {
  }

  std::filesystem::path package_file = install_context.source;
  std::filesystem::path install_location = install_context.target;

  std::string package_format = package_file.extension().string();
  if (package_format == std::string(FILE_NAME_EXT_SEP) + PACKAGEINFO_PACKAGE_FORMAT_ZIP) {
    std::error_code ec = InstallZipPackage(std::move(package_file), std::move(install_location));
    if (ec)
      return ec;
  } else {
    return make_selfupdate_error(SUE_UnsupportedPackageFormat);
  }

  std::filesystem::remove(package_file);

#ifdef _WIN32
  std::wstringstream ss;
  ss << L"cmd /C ping 127.0.0.1 -n 10 >Nul & Del /F /Q \"" << exe_path.native() << L"\" & RMDIR /Q \""
     << exe_path.parent_path().native() << L"\"";
  std::wstring cmd = ss.str();
#ifdef _DEBUG
  wprintf(L"Self delete command: %s\n", cmd.c_str());
#endif
  STARTUPINFO si = {sizeof(STARTUPINFO)};
  PROCESS_INFORMATION pi = {};
  ::CreateProcess(nullptr, cmd.data(), nullptr, nullptr, false, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
  ::CloseHandle(pi.hThread);
  ::CloseHandle(pi.hProcess);
#else
  std::filesystem::remove(exe_path);
#endif

  std::filesystem::path launch_path = install_location / install_context.launch_file;
  std::filesystem::permissions(launch_path,
                               std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec |
                                   std::filesystem::perms::others_exec,
                               std::filesystem::perm_options::add);
  boost::process::spawn(launch_path.string(), boost::process::start_dir(launch_path.parent_path().string()));

  return {};
}

} // namespace selfupdate
