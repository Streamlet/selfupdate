#include "../common.h"
#include <selfupdate/updater.h>
#include <xl/http>
#include <xl/json>
#include <xl/log>
#include <xl/native_string>

namespace selfupdate {

namespace {

typedef std::map<std::string, std::string> StringMap;

XL_JSON_BEGIN(PackageInfoInternal)
  XL_JSON_MEMBER(std::string, package_name)
  XL_JSON_MEMBER(bool, has_new_version)
  XL_JSON_MEMBER(std::string, package_version)
  XL_JSON_MEMBER(bool, force_update)
  XL_JSON_MEMBER(std::string, package_url)
  XL_JSON_MEMBER(unsigned long long, package_size)
  XL_JSON_MEMBER(std::string, package_format)
  XL_JSON_MEMBER(StringMap, package_hash)
  XL_JSON_MEMBER(std::string, update_title)
  XL_JSON_MEMBER(std::string, update_description)
XL_JSON_END()

const unsigned QUERY_TIMEOUT = 10000;

} // namespace

bool Query(const std::string &query_url,
           const std::multimap<std::string, std::string> &headers,
           const std::string &query_body,
           PackageInfo &package_info) {
  XL_LOG_INFO("Querying: ", query_url, ", headers: ", headers.size(), ", body: ", query_body);
  xl::http::Request request;
  request.url = query_url;
  request.method = query_body.empty() ? xl::http::METHOD_GET : xl::http::METHOD_POST;
  xl::http::Response response;
  std::string response_body;
  response.body = xl::http::buffer_writer(&response_body);
  xl::http::Option option;
  option.user_agent = SELFUPDATE_USER_AGENT;

  unsigned status = xl::http::send(request, &response, &option);
  if (status != 200) {
    XL_LOG_ERROR("Querying failed. http status/error: ", status);
    return false;
  }
  XL_LOG_INFO("Quering succeeded. Result: ", response_body);

  PackageInfoInternal json;
  if (!json.json_parse(response_body.c_str())) {
    XL_LOG_ERROR("Parsing json failed.");
    return false;
  }

  package_info.package_name = std::move(json.package_name);
  package_info.has_new_version = json.has_new_version;
  package_info.package_version = std::move(json.package_version);
  package_info.force_update = std::move(json.force_update);
  package_info.package_url = std::move(json.package_url);
  package_info.package_size = std::move(json.package_size);
  package_info.package_format = std::move(json.package_format);
  package_info.package_hash = std::move(json.package_hash);
  package_info.update_title = std::move(json.update_title);
  package_info.update_description = std::move(json.update_description);

  if (!package_info.has_new_version) {
    XL_LOG_INFO("No new version.");
    return true;
  }
  if (package_info.package_format != PACKAGEINFO_PACKAGE_FORMAT_ZIP) {
    XL_LOG_ERROR("Unsupported package format: ", package_info.package_format);
    return false;
  }
  for (const auto &item : package_info.package_hash) {
    if (item.first != PACKAGEINFO_PACKAGE_HASH_ALGO_MD5 && item.first != PACKAGEINFO_PACKAGE_HASH_ALGO_SHA1 &&
        item.first != PACKAGEINFO_PACKAGE_HASH_ALGO_SHA224 && item.first != PACKAGEINFO_PACKAGE_HASH_ALGO_SHA256 &&
        item.first != PACKAGEINFO_PACKAGE_HASH_ALGO_SHA384 && item.first != PACKAGEINFO_PACKAGE_HASH_ALGO_SHA512) {
      XL_LOG_ERROR("Unsupported hash algorithm: ", item.first);
      return false;
    }
  }
  XL_LOG_INFO("New version found: ", package_info.package_version, ", url: ", package_info.package_url);
  return true;
}

} // namespace selfupdate
