#pragma once

#include <functional>
#include <string>
#include <map>
#include <system_error>

namespace selfupdate {

struct PackageInfo {
  std::string package_name;
  std::string package_version;
  std::string package_url;
  size_t package_size;
  std::string package_format;
  std::map<std::string, std::string> package_hash;
  std::string update_title;
  std::string update_description;
};

void Initialize(const std::string &app_name, const std::string &user_agent = "");

std::error_code Query(const std::string &query_url, const std::string &query_body, PackageInfo &package_info);

typedef std::function<void(unsigned long long downloaded_bytes, unsigned long long total_bytes)>
    DownloadProgressMonitor;
std::error_code Download(const PackageInfo &package_info, DownloadProgressMonitor download_progress_monitor);

std::error_code Install(const PackageInfo &package_info, const std::string &install_dir = ".");

} // namespace selfupdate
