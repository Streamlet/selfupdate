#pragma once

#include <functional>
#include <string_view>

namespace selfupdate {

struct PackageInfo {
  std::string_view title;
  std::string_view description;
  std::string_view server_version;
  std::string_view package_url;
  std::string_view package_format;
};

PackageInfo query(std::string_view package_name,
                  std::string_view client_version);
typedef std::function<void(unsigned long long downloaded_bytes,
                           unsigned long long total_bytes)>
    DownloadProgressMonitor;
void download(const PackageInfo& package_url, DownloadProgressMonitor monitor);

void install(const PackageInfo& PackageInfo);

}  // namespace selfupdate
