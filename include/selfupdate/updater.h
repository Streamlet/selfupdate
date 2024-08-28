#pragma once

#include <functional>
#include <map>
#include <string>

#ifdef _WIN32
#include <tchar.h>
#else
#ifndef TCHAR
#define TCHAR char
#endif
#endif

namespace selfupdate {

struct PackageInfo {
  std::string package_name;
  bool has_new_version = false;
  std::string package_version;
  bool force_update = false;
  std::string package_url;
  unsigned long long package_size = 0;
  std::string package_format;
  std::map<std::string, std::string> package_hash;
  std::string update_title;
  std::string update_description;
};

bool Query(const std::string &query_url,
           const std::multimap<std::string, std::string> &headers,
           const std::string &query_body,
           PackageInfo &package_info);

typedef std::function<void(unsigned long long downloaded_bytes, unsigned long long total_bytes)>
    DownloadProgressMonitor;
bool Download(const PackageInfo &package_info, DownloadProgressMonitor download_progress_monitor);

bool Install(const PackageInfo &package_info,
             const TCHAR *installer_path = nullptr,    // default to the executable path
             const TCHAR *install_location = nullptr); // default to the executable directory

#ifdef _WIN32
bool IsNewVersionFirstLaunched(int argc, const TCHAR *argv[]);
bool IsNewVersionFirstLaunched(const TCHAR *command_line);
bool IsForceUpdated(int argc, const TCHAR *argv[]);
bool IsForceUpdated(const TCHAR *command_line);
#else
bool IsNewVersionFirstLaunched(int argc, const char *argv[]);
bool IsForceUpdated(int argc, const char *argv[]);
#endif

} // namespace selfupdate

#ifndef SELFUPDATE_USER_AGENT
#define SELFUPDATE_USER_AGENT "selfupdate"
#endif
