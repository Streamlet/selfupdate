#pragma once

#include <functional>
#include <string>
#include <system_error>

namespace selfupdate {

struct PackageInfo {
  std::string title;
  std::string description;
  std::string server_version;
  std::string package_url;
  std::string package_format;
};

std::error_code Query(const std::string &query_url, const std::string &query_body, PackageInfo &package_info);
typedef std::function<void(unsigned long long downloaded_bytes, unsigned long long total_bytes)>
    DownloadProgressMonitor;
void Download(const PackageInfo &package_url, DownloadProgressMonitor monitor);

void Install(const PackageInfo &package_info);

} // namespace selfupdate
