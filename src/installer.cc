#include "../include/selfupdate/installer.h"
#include "common.h"
#include <cstdio>
#include <filesystem>
#include <sstream>
#include <xl/cmdline_options>
#include <xl/encoding>
#include <xl/log>
#include <xl/native_string>
#include <xl/process>
#include <xl/zip>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace selfupdate {

struct InstallContext {
  int wait_pid = 0;
  bool force_update = false;
  std::filesystem::path source;
  std::filesystem::path target;
  std::filesystem::path launch_file;
};

namespace {

const InstallContext *IsInstallMode(const xl::cmdline_options::parsed_options &options) {
  if (!options.has(_T(INSTALLER_ARGUMENT_UPDATE))) {
    return nullptr;
  }
  if (!options.has(_T(INSTALLER_ARGUMENT_WAIT_PID)) || !options.has(_T(INSTALLER_ARGUMENT_SOURCE)) ||
      !options.has(_T(INSTALLER_ARGUMENT_TARGET)) || !options.has(_T(INSTALLER_ARGUMENT_LAUNCH_FILE))) {
    return nullptr;
  }

  int wait_pid = options.get_as<int>(_T(INSTALLER_ARGUMENT_WAIT_PID));
  bool force_update = options.get_as<bool>(_T(INSTALLER_ARGUMENT_FORCE_UPDATE));
  xl::native_string source = options.get(_T(INSTALLER_ARGUMENT_SOURCE));
  xl::native_string target = options.get(_T(INSTALLER_ARGUMENT_TARGET));
  xl::native_string launch_file = options.get(_T(INSTALLER_ARGUMENT_LAUNCH_FILE));

  auto trim_quote = [](xl::native_string &s) -> xl::native_string & {
    s.erase(0, s.find_first_not_of(_T('"'), 0));
    s.erase(s.find_last_not_of(_T('"')) + 1);
    return s;
  };
  trim_quote(source);
  trim_quote(target);
  trim_quote(launch_file);

  InstallContext *install_context = new InstallContext;
  install_context->wait_pid = wait_pid;
  install_context->force_update = force_update;
  install_context->source = source;
  install_context->target = target;
  install_context->launch_file = launch_file;
  return install_context;
}

} // namespace

const InstallContext *IsInstallMode(int argc, const TCHAR *argv[]) {
  return std::move(IsInstallMode(xl::cmdline_options::parse(argc, argv)));
}

#ifdef _WIN32
const InstallContext *IsInstallMode(const TCHAR *command_line) {
  return std::move(IsInstallMode(xl::cmdline_options::parse(command_line)));
}
#endif

namespace {

const char *INSTALL_LOCATION_OLD_SUFFIX = ".old";
const char *INSTALL_LOCATION_NEW_SUFFIX = ".new";
const int INSTALL_WAIT_FOR_MAIN_PROCESS = 10000;

bool InstallZipPackage(const std::filesystem::path &package_file, const std::filesystem::path &install_location) {
  XL_LOG_INFO(_T("Installing zip package, from: "), package_file.c_str(), _T(", to: "), install_location.c_str());

  std::filesystem::path install_location_old = install_location;
  install_location_old += INSTALL_LOCATION_OLD_SUFFIX;
  std::filesystem::remove_all(install_location_old);
  std::filesystem::path install_location_new = install_location;
  install_location_new += INSTALL_LOCATION_NEW_SUFFIX;
  std::filesystem::remove_all(install_location_new);

  XL_LOG_INFO(_T("Extracting package, from: "), package_file.c_str(), _T(", to: "), install_location_new.c_str());
  if (!xl::zip::extract(package_file.c_str(), install_location_new.c_str())) {
    XL_LOG_INFO(_T("Extract package failed, from: "), package_file.c_str(), _T(", to: "), install_location_new.c_str());
    return false;
  }

  XL_LOG_INFO(_T("Renaming old installation, from: "), install_location.c_str(), _T(", to: "),
              install_location_old.c_str());
  std::error_code ec;
  for (int i = 0; i < INSTALL_WAIT_FOR_MAIN_PROCESS && std::filesystem::exists(install_location); ++i) {
    std::filesystem::rename(install_location, install_location_old, ec);
    if (ec && i + 1 < INSTALL_WAIT_FOR_MAIN_PROCESS) {
      XL_LOG_WARN("Renaming old installation failed, retrying. From: ", install_location.u8string(),
                  ", to: ", install_location_old.u8string(), ", error category: ", ec.category().name(),
                  ", code: ", ec.value(), ", message: ", ec.message());
      xl::process::sleep(1000);
    }
  }
  if (std::filesystem::exists(install_location)) {
    XL_LOG_ERROR("Renaming old installation failed. From: ", install_location.u8string(),
                 ", to: ", install_location_old.u8string(), ", error category: ", ec.category().name(),
                 ", code: ", ec.value(), ", message: ", ec.message());
    return false;
  }

  XL_LOG_INFO(_T("Renaming new installation, from: "), install_location_new.c_str(), _T(", to: "),
              install_location.c_str());
  std::filesystem::rename(install_location_new, install_location, ec);
  if (ec) {
    XL_LOG_ERROR("Renaming new installation failed. From: ", install_location_new.u8string(),
                 ", to: ", install_location.u8string(), ", error category: ", ec.category().name(),
                 ", code: ", ec.value(), ", message: ", ec.message());
    return false;
  }

  if (!std::filesystem::exists(install_location)) {
    XL_LOG_ERROR(_T("New installation missing: "), install_location.c_str());
    return false;
  }

  if (std::filesystem::exists(install_location_old)) {
    XL_LOG_INFO(_T("Copying extra files from old installation, from: "), install_location_old.c_str(), _T(", to: "),
                install_location.c_str());
    for (const std::filesystem::directory_entry &dir_entry :
         std::filesystem::recursive_directory_iterator(install_location_old)) {
      std::filesystem::path file_in_new_path =
          install_location / dir_entry.path().lexically_relative(install_location_old);
      if (!std::filesystem::exists(file_in_new_path)) {
        XL_LOG_INFO(_T("Copying"), dir_entry.path().c_str(), _T("to"), file_in_new_path.c_str());
        std::filesystem::rename(dir_entry.path(), file_in_new_path);
      }
    }
    XL_LOG_INFO(_T("Deleting old installation: "), install_location_old.c_str());
    std::filesystem::remove_all(install_location_old);
  }

  XL_LOG_INFO("Install zip package OK");
  return true;
}

} // namespace

bool DoInstall(const InstallContext *install_context) {
  XL_LOG_INFO(_T("Installing, from: "), install_context->source.c_str(), _T(", to: "), install_context->target.c_str());

  auto install_context_ptr = std::unique_ptr<const InstallContext>(install_context);
  auto exe_path = xl::process::executable_path();
  if (exe_path.find(install_context->target) == 0) {
    XL_LOG_ERROR(_T("Installer path error, installer is at: "), exe_path.c_str(), _T(", while install target is: "),
                 install_context->target.c_str());
    return false;
  }

  if (!xl::process::wait(install_context->wait_pid, INSTALL_WAIT_FOR_MAIN_PROCESS)) {
    xl::process::wait(install_context->wait_pid);
  }

  std::filesystem::path package_file = install_context->source;
  std::filesystem::path install_location = install_context->target;

  xl::native_string package_format = package_file.extension().native();
  if (package_format == _T(FILE_NAME_EXT_SEP PACKAGEINFO_PACKAGE_FORMAT_ZIP)) {
    if (!InstallZipPackage(std::move(package_file), std::move(install_location))) {
      XL_LOG_ERROR(_T("Install package failed, from: "), install_context->source.c_str(), _T(", to: "),
                   install_context->target.c_str());
      return false;
    }
  } else {
    XL_LOG_ERROR(_T("Unsupported package format: "), package_format);
    return false;
  }

  std::filesystem::remove(package_file);

  {
#ifdef _WIN32
    xl::native_string_stream ss;
    ss << _T("cmd /C ping 127.0.0.1 -n 10 >Nul & Del /F /Q \"") << exe_path << _T("\" & RMDIR /Q \"")
       << std::filesystem::path(exe_path).parent_path().native() << _T("\"");
    xl::native_string cmd = ss.str();
    XL_LOG_INFO(_T("Self-delete command: "), cmd);
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
    XL_LOG_INFO(_T("Launching new version. Command line: "), launch_path.c_str(),
                _T("--" INSTALLER_ARGUMENT_NEW_VERSION), _T("--" INSTALLER_ARGUMENT_FORCE_UPDATE),
                install_context->force_update ? _T("1") : _T("0"));
    long pid = xl::process::start(launch_path.native(),
                                  {
                                      _T("--" INSTALLER_ARGUMENT_NEW_VERSION),
                                      _T("--" INSTALLER_ARGUMENT_FORCE_UPDATE),
                                      install_context->force_update ? _T("1") : _T("0"),
                                  },
                                  launch_path.parent_path().native());
    if (pid == 0) {
      XL_LOG_ERROR(_T("Launch new version failed. Command line: "), launch_path.c_str(),
                   _T("--" INSTALLER_ARGUMENT_NEW_VERSION));
      return false;
    }
  }

  XL_LOG_INFO("Install package OK");
  return true;
}

} // namespace selfupdate
