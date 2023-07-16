#include "../include/selfupdate/installer.h"
#include "../utility/cmdline_options.h"
#include "../utility/native_string.h"
#include "common.h"
#include <boost/algorithm/string.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/process.hpp>
#include <chrono>
#include <filesystem>
#include <locale>
#include <sstream>
#include <stdio.h>
#include <zlibwrap/zlibwrap.h>
#ifdef _WIN32
#include "../utility/encoding.h"
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace selfupdate {

struct InstallContext {
  int wait_pid = 0;
  std::filesystem::path source;
  std::filesystem::path target;
  std::filesystem::path launch_file;
};

namespace {

const InstallContext *IsInstallMode(const cmdline_options::ParsedOption &options) {
  if (!options.has(_T(INSTALLER_ARGUMENT_UPDATE))) {
    return nullptr;
  }
  if (!options.has(_T(INSTALLER_ARGUMENT_WAIT_PID)) || !options.has(_T(INSTALLER_ARGUMENT_SOURCE)) ||
      !options.has(_T(INSTALLER_ARGUMENT_TARGET)) || !options.has(_T(INSTALLER_ARGUMENT_LAUNCH_FILE))) {
    return nullptr;
  }

  int wait_pid = options.get_as<int>(_T(INSTALLER_ARGUMENT_WAIT_PID));
  native_string source = options.get(_T(INSTALLER_ARGUMENT_SOURCE));
  native_string target = options.get(_T(INSTALLER_ARGUMENT_TARGET));
  native_string launch_file = options.get(_T(INSTALLER_ARGUMENT_LAUNCH_FILE));

  auto is_quote = [](TCHAR ch) {
    return ch == _T('"');
  };
  boost::algorithm::trim_if(source, is_quote);
  boost::algorithm::trim_if(target, is_quote);
  boost::algorithm::trim_if(launch_file, is_quote);

  InstallContext *install_context = new InstallContext;
  install_context->wait_pid = wait_pid;
  install_context->source = source;
  install_context->target = target;
  install_context->launch_file = launch_file;
  return install_context;
}

} // namespace

const InstallContext *IsInstallMode(int argc, const TCHAR *argv[]) {
  return std::move(IsInstallMode(cmdline_options::parse(argc, argv)));
}

#ifdef _WIN32
const InstallContext *IsInstallMode(const TCHAR *command_line) {
  return std::move(IsInstallMode(cmdline_options::parse(command_line)));
}
#endif

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

  if (!zlibwrap::ZipExtract(package_file.c_str(), install_location_new.c_str()))
    return make_selfupdate_error(SUE_PackageExtractError);

  std::error_code ec;
  for (int i = 0; i < INSTALL_WAIT_FOR_MAIN_PROCESS && std::filesystem::exists(install_location); ++i) {
    std::filesystem::rename(install_location, install_location_old, ec);
    if (ec) {
#if _WIN32
      ::Sleep(1000);
#else
      ::sleep(1);
#endif
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

std::error_code DoInstall(const InstallContext *install_context) {
  auto install_context_ptr = std::unique_ptr<const InstallContext>(install_context);
  auto exe_path = boost ::dll::program_location();
  if (exe_path.native().find(install_context->target) == 0)
    return make_selfupdate_error(SUE_RunInstallerPositionError);

  try {
    boost::process::child main_process(static_cast<boost::process::pid_t>(install_context->wait_pid));
    if (main_process.valid() && main_process.running()) {
      if (!main_process.wait_for(std::chrono::seconds(INSTALL_WAIT_FOR_MAIN_PROCESS))) {
        main_process.terminate();
      }
    }
  } catch (boost::process::process_error e) {
  }

  std::filesystem::path package_file = install_context->source;
  std::filesystem::path install_location = install_context->target;

  native_string package_format = package_file.extension().native();
  if (package_format == _T(FILE_NAME_EXT_SEP PACKAGEINFO_PACKAGE_FORMAT_ZIP)) {
    std::error_code ec = InstallZipPackage(std::move(package_file), std::move(install_location));
    if (ec)
      return ec;
  } else {
    return make_selfupdate_error(SUE_UnsupportedPackageFormat);
  }

  std::filesystem::remove(package_file);

  {
#ifdef _WIN32
    native_string_stream ss;
    ss << _T("cmd /C ping 127.0.0.1 -n 10 >Nul & Del /F /Q \"") << exe_path.native() << _T("\" & RMDIR /Q \"")
       << exe_path.parent_path().native() << _T("\"");
    native_string cmd = ss.str();
    STARTUPINFO si = {sizeof(STARTUPINFO)};
    PROCESS_INFORMATION pi = {};
    ::CreateProcess(nullptr, cmd.data(), nullptr, nullptr, false, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    ::CloseHandle(pi.hThread);
    ::CloseHandle(pi.hProcess);
#else
    std::filesystem::remove(exe_path);
#endif
  }

  {
    std::filesystem::path launch_path = install_location / install_context->launch_file;
    std::filesystem::permissions(launch_path,
                                 std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec |
                                     std::filesystem::perms::others_exec,
                                 std::filesystem::perm_options::add);
    native_string_stream ss;
    ss << launch_path.native();
    ss << _T(" --" INSTALLER_ARGUMENT_NEW_VERSION);
    native_string cmd = ss.str();
    boost::process::spawn(cmd, boost::process::start_dir(launch_path.parent_path().native()));
  }

  return {};
}

} // namespace selfupdate
