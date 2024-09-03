#include "../common.h"
#include "zip_installer.h"
#include <cstdio>
#include <selfupdate/installer.h>
#include <sstream>
#include <xl/cmdline_options>
#include <xl/encoding>
#include <xl/file>
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
  xl::native_string source;
  xl::native_string target;
  xl::native_string launch_file;
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

const int INSTALL_WAIT_FOR_MAIN_PROCESS = 10000;

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

  xl::native_string package_file = install_context->source;
  xl::native_string install_location = install_context->target;

  xl::native_string package_format = xl::path::extname(package_file.c_str());
  if (package_format == _T(FILE_NAME_EXT_SEP PACKAGEINFO_PACKAGE_FORMAT_ZIP)) {
    if (!InstallZipPackage(package_file, install_location)) {
      XL_LOG_ERROR(_T("Install package failed, from: "), install_context->source.c_str(), _T(", to: "),
                   install_context->target.c_str());
      return false;
    }
  } else {
    XL_LOG_ERROR(_T("Unsupported package format: "), package_format);
    return false;
  }
  xl::fs::remove(package_file.c_str());

  {
#ifdef _WIN32
    xl::native_string_stream ss;
    ss << _T("cmd /C ping 127.0.0.1 -n 10 >Nul & Del /F /Q \"") << exe_path << _T("\" & RMDIR /Q \"")
       << xl::path::dirname(exe_path.c_str()) << _T("\"");
    xl::native_string cmd = ss.str();
    XL_LOG_INFO(_T("Self-delete command: "), cmd);
    STARTUPINFO si = {sizeof(STARTUPINFO)};
    PROCESS_INFORMATION pi = {};
    ::CreateProcess(nullptr, cmd.data(), nullptr, nullptr, false, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    ::CloseHandle(pi.hThread);
    ::CloseHandle(pi.hProcess);
#else
    xl::fs::remove(exe_path.c_str());
#endif
  }

  {
    xl::native_string launch_path = xl::path::join(install_location, install_context->launch_file);
    XL_LOG_INFO(_T("Launching new version. Command line: "), launch_path.c_str(),
                _T("--" INSTALLER_ARGUMENT_NEW_VERSION), _T("--" INSTALLER_ARGUMENT_FORCE_UPDATE),
                install_context->force_update ? _T("1") : _T("0"));
    long pid = xl::process::start(launch_path.c_str(),
                                  {
                                      _T("--" INSTALLER_ARGUMENT_NEW_VERSION),
                                      _T("--" INSTALLER_ARGUMENT_FORCE_UPDATE),
                                      install_context->force_update ? _T("1") : _T("0"),
                                  },
                                  install_location);
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
