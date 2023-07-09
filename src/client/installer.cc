#include "../include/selfupdate/installer.h"
#include "../utility/cmdline_options.h"
#include "common.h"
#include "native_string.h"
#include <boost/algorithm/string.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/process.hpp>
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

struct InstallContext {
  int wait_pid = 0;
  std::filesystem::path source;
  std::filesystem::path target;
  std::filesystem::path launch_file;
};

namespace {

template <typename CharType>
const InstallContext *MakeInstallContent(std::basic_string<CharType> wait_pid,
                                         std::basic_string<CharType> source,
                                         std::basic_string<CharType> target,
                                         std::basic_string<CharType> launch_file) {
  auto is_quote = [](CharType ch) {
    return ch == '"';
  };
  boost::algorithm::trim_if(source, is_quote);
  boost::algorithm::trim_if(target, is_quote);
  boost::algorithm::trim_if(launch_file, is_quote);

  InstallContext *install_context = new InstallContext;
  install_context->wait_pid = std::stoi(wait_pid);
  install_context->source = source;
  install_context->target = target;
  install_context->launch_file = launch_file;
  return install_context;
}

const InstallContext *IsInstallMode(const std::map<std::string, std::string> cmdline_options) {
  if (cmdline_options.find(INSTALLER_ARGUMENT_UPDATE) == cmdline_options.end()) {
    return nullptr;
  }
  auto wait_pid = cmdline_options.find(INSTALLER_ARGUMENT_WAIT_PID);
  auto source = cmdline_options.find(INSTALLER_ARGUMENT_SOURCE);
  auto target = cmdline_options.find(INSTALLER_ARGUMENT_TARGET);
  auto launch_file = cmdline_options.find(INSTALLER_ARGUMENT_LAUNCH_FILE);
  if (wait_pid == cmdline_options.end() || wait_pid->second.empty() || source == cmdline_options.end() ||
      target == cmdline_options.end() || launch_file == cmdline_options.end()) {
    return nullptr;
  }
  return MakeInstallContent<char>(wait_pid->second, std::move(source->second), std::move(target->second),
                                  std::move(launch_file->second));
}

const InstallContext *IsInstallMode(const std::map<std::wstring, std::wstring> cmdline_options) {
  if (cmdline_options.find(_L(INSTALLER_ARGUMENT_UPDATE)) == cmdline_options.end()) {
    return nullptr;
  }
  auto wait_pid = cmdline_options.find(_L(INSTALLER_ARGUMENT_WAIT_PID));
  auto source = cmdline_options.find(_L(INSTALLER_ARGUMENT_SOURCE));
  auto target = cmdline_options.find(_L(INSTALLER_ARGUMENT_TARGET));
  auto launch_file = cmdline_options.find(_L(INSTALLER_ARGUMENT_LAUNCH_FILE));
  if (wait_pid == cmdline_options.end() || wait_pid->second.empty() || source == cmdline_options.end() ||
      target == cmdline_options.end() || launch_file == cmdline_options.end()) {
    return nullptr;
  }
  return MakeInstallContent<wchar_t>(wait_pid->second, std::move(source->second), std::move(target->second),
                                     std::move(launch_file->second));
}

} // namespace

const InstallContext *IsInstallMode(int argc, const char *argv[]) {
  return IsInstallMode(std::move(cmdline_options::parse(argc, argv)));
}

const InstallContext *IsInstallMode(int argc, const wchar_t *argv[]) {
  return IsInstallMode(std::move(cmdline_options::parse(argc, argv)));
}

#ifdef _WIN32
const InstallContext *IsInstallMode(const char *command_line) {
  return IsInstallMode(std::move(cmdline_options::parse(command_line)));
}
const InstallContext *IsInstallMode(const wchar_t *command_line) {
  return IsInstallMode(std::move(cmdline_options::parse(command_line)));
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

  if (!zlibwrap::ZipExtract(package_file.string().c_str(), install_location_new.string().c_str()))
    return make_selfupdate_error(SUE_PackageExtractError);

  std::error_code ec;
  for (int i = 0; i < INSTALL_WAIT_FOR_MAIN_PROCESS && std::filesystem::exists(install_location); ++i) {
    std::filesystem::rename(install_location, install_location_old, ec);
    if (ec) {
#if _WIN32
      std::this_thread::sleep_for(std::chrono::seconds(1));
#else
      break;
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

  tstring package_format = package_file.extension().native();
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
    tstringstream ss;
    ss << _T("cmd /C ping 127.0.0.1 -n 10 >Nul & Del /F /Q \"") << exe_path.native() << _T("\" & RMDIR /Q \"")
       << exe_path.parent_path().native() << _T("\"");
    tstring cmd = ss.str();
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
    tstringstream ss;
    ss << launch_path.native();
    ss << _T(" --" INSTALLER_ARGUMENT_NEW_VERSION);
    tstring cmd = ss.str();
    boost::process::spawn(cmd, boost::process::start_dir(launch_path.parent_path().native()));
  }

  return {};
}

} // namespace selfupdate
