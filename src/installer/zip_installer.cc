#include "zip_installer.h"
#include <xl/file>
#include <xl/log>
#include <xl/process>
#include <xl/zip>

namespace selfupdate {

namespace {

const TCHAR *INSTALL_LOCATION_OLD_SUFFIX = _T(".old");
const TCHAR *INSTALL_LOCATION_NEW_SUFFIX = _T(".new");
const int INSTALL_WAIT_FOR_MAIN_PROCESS = 10000;

} // namespace

bool InstallZipPackage(const xl::native_string &package_file, const xl::native_string &install_location) {
  XL_LOG_INFO(_T("Installing zip package, from: "), package_file.c_str(), _T(", to: "), install_location.c_str());

  xl::native_string install_location_old = install_location + INSTALL_LOCATION_OLD_SUFFIX;
  xl::fs::remove_all(install_location_old.c_str());
  xl::native_string install_location_new = install_location + INSTALL_LOCATION_NEW_SUFFIX;
  xl::fs::remove_all(install_location_new.c_str());

  XL_LOG_INFO(_T("Extracting package, from: "), package_file.c_str(), _T(", to: "), install_location_new.c_str());
  if (!xl::zip::extract(package_file.c_str(), install_location_new.c_str())) {
    XL_LOG_INFO(_T("Extract package failed, from: "), package_file.c_str(), _T(", to: "), install_location_new.c_str());
    return false;
  }

  XL_LOG_INFO(_T("Renaming old installation, from: "), install_location.c_str(), _T(", to: "),
              install_location_old.c_str());
  std::error_code ec;
  for (int i = 0; i < INSTALL_WAIT_FOR_MAIN_PROCESS && xl::fs::exists(install_location.c_str()); ++i) {
    if (!xl::fs::move(install_location.c_str(), install_location_old.c_str()) &&
        i + 1 < INSTALL_WAIT_FOR_MAIN_PROCESS) {
      XL_LOG_WARN("Renaming old installation failed, retrying. (", install_location, " => ", install_location_old, ")");
      xl::process::sleep(1000);
    }
  }
  if (xl::fs::exists(install_location.c_str())) {
    XL_LOG_WARN("Renaming old installation failed. (", install_location, " => ", install_location_old, ")");
    return false;
  }

  XL_LOG_INFO("Renaming new installation. (", install_location_new, " => ", install_location, ")");
  xl::fs::move(install_location_new.c_str(), install_location.c_str());
  if (ec) {
    XL_LOG_ERROR("Renaming new installation failed. ()", install_location_new, " => ", install_location, ")");
    return false;
  }

  if (!xl::fs::exists(install_location.c_str())) {
    XL_LOG_ERROR(_T("New installation missing: "), install_location.c_str());
    return false;
  }

  if (xl::fs::exists(install_location_old.c_str())) {
    XL_LOG_INFO(_T("Copying extra files from old installation, from: "), install_location_old.c_str(), _T(", to: "),
                install_location.c_str());
    xl::fs::enum_dir(
        install_location_old.c_str(),
        [&install_location_old, &install_location](const xl::native_string &path, bool is_dir) -> bool {
          xl::native_string new_path = xl::path::join(install_location.c_str(), path.c_str());
          if (!xl::fs::exists(new_path.c_str())) {
            xl::native_string old_path = xl::path::join(install_location_old.c_str(), path.c_str());
            XL_LOG_INFO("Copying", path, "to", install_location);
            return xl::fs::move(old_path.c_str(), new_path.c_str());
          }
          return true;
        },
        true);

    XL_LOG_INFO("Deleting old installation: ", install_location_old.c_str());
    xl::fs::remove_all(install_location_old.c_str());
  }

  XL_LOG_INFO("Install zip package OK");
  return true;
}

} // namespace selfupdate
