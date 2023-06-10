#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <system_error>

namespace selfupdate {

struct PackageInfo {
  std::string package_name;
  bool has_new_version = false;
  std::string package_version;
  bool force_update = false;
  std::string package_url;
  size_t package_size = 0;
  std::string package_format;
  std::map<std::string, std::string> package_hash;
  std::string update_title;
  std::string update_description;
};

std::error_code Query(const std::string &query_url, const std::string &query_body, PackageInfo &package_info);

typedef std::function<void(unsigned long long downloaded_bytes, unsigned long long total_bytes)>
    DownloadProgressMonitor;
std::error_code Download(const PackageInfo &package_info, DownloadProgressMonitor download_progress_monitor);

std::error_code Install(const PackageInfo &package_info,
                        std::filesystem::path installer_path,
                        std::filesystem::path install_location = {}, // default to executable directory
                        std::filesystem::path launch_file = {});     // default to executable filename

} // namespace selfupdate

#ifndef SELFUPDATE_USER_AGENT
#define SELFUPDATE_USER_AGENT "selfupdate"
#endif
