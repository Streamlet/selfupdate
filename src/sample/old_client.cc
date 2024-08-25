#include <cmath>
#include <selfupdate/installer.h>
#include <selfupdate/selfupdate.h>
#include <xl/log_setup>
#include <xl/native_string>

int _tmain(int argc, const TCHAR *argv[]) {
  xl::log::setup(_T("old_client"));

  if (selfupdate::IsNewVersionFirstLaunched(argc, argv)) {
    XL_LOG_ERROR("ERROR! old_client lauched as new_client, previous upgrading failed!");
    return -1;
  }

  const selfupdate::InstallContext *install_context = selfupdate::IsInstallMode(argc, argv);
  if (install_context != nullptr) {
    XL_LOG_INFO("Installing...");
    std::error_code ec = selfupdate::DoInstall(install_context);
    if (ec) {
      return -1;
    }
    return 0;
  }

  XL_LOG_INFO("old_client launched.");

  XL_LOG_INFO("Step 1: query package info");
  selfupdate::PackageInfo package_info;
  std::error_code ec = selfupdate::Query("http://localhost:8080/sample_package/1.0", {}, "", package_info);
  if (ec) {
    return -1;
  }

  XL_LOG_INFO("package_name: ", package_info.package_name);
  XL_LOG_INFO("has_new_version: ", package_info.has_new_version);
  XL_LOG_INFO("package_version: ", package_info.package_version);
  XL_LOG_INFO("force_update: ", package_info.force_update);
  XL_LOG_INFO("package_url: ", package_info.package_url);
  XL_LOG_INFO("package_size: ", package_info.package_size);
  XL_LOG_INFO("package_format: ", package_info.package_format);
  for (const auto &item : package_info.package_hash) {
    XL_LOG_INFO("package_hash: ", item.first, ", ", item.second);
  }
  XL_LOG_INFO("update_title: ", package_info.update_title);
  XL_LOG_INFO("update_description: ", package_info.update_description);

  XL_LOG_INFO("Step 2: download package");
  ec = selfupdate::Download(package_info, [](unsigned long long downloaded_bytes, unsigned long long total_bytes) {
    XL_LOG_INFO(std::to_string(round(downloaded_bytes * 10000.0 / total_bytes) / 100) + "%,",
                std::to_string(downloaded_bytes) + "/" + std::to_string(total_bytes));
  });

  if (ec) {
    return -1;
  }

  XL_LOG_INFO("Step 3: install package");
  ec = selfupdate::Install(package_info);
  if (ec) {
    return -1;
  }

  XL_LOG_INFO("Old client quitting.");
  return 0;
}
