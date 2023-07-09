#include "../include/selfupdate/selfupdate.h"
#include "../utility/http_client.h"
#include "common.h"
#include <boost/json/src.hpp>

namespace selfupdate {

namespace {

const unsigned QUERY_TIMEOUT = 10000;

const char *PACKAGEINFO_PACKAGE_NAME = "package_name";
const char *PACKAGEINFO_HAS_NEW_VERSION = "has_new_version";
const char *PACKAGEINFO_PACKAGE_VERSION = "package_version";
const char *PACKAGEINFO_FORCE_UPDATE = "force_update";
const char *PACKAGEINFO_PACKAGE_URL = "package_url";
const char *PACKAGEINFO_PACKAGE_SIZE = "package_size";
const char *PACKAGEINFO_PACKAGE_FORMAT = "package_format";
const char *PACKAGEINFO_PACKAGE_HASH = "package_hash";
const char *PACKAGEINFO_UPDATE_TITLE = "update_title";
const char *PACKAGEINFO_UPDATE_DESCRIPTION = "update_description";

} // namespace

std::error_code Query(const std::string &query_url, const std::string &query_body, PackageInfo &package_info) {
  HttpClient http_client(SELFUPDATE_USER_AGENT);
  unsigned status = 0;
  std::string response;
  std::error_code ec;
  if (query_body.empty())
    ec = http_client.Get(query_url, {}, &status, nullptr, &response, QUERY_TIMEOUT);
  else
    ec = http_client.Post(query_url, {}, query_body, &status, nullptr, &response, QUERY_TIMEOUT);
  if (ec || status != 200)
    return ec;

  boost::json::value jv = boost::json::parse(response, ec);
  if (ec)
    return ec;
  if (!jv.is_object())
    return make_selfupdate_error(SUE_PackageInfoFormatError);
  boost::json::object doc = jv.as_object();
  auto package_name = doc.find(PACKAGEINFO_PACKAGE_NAME);
  auto has_new_version = doc.find(PACKAGEINFO_HAS_NEW_VERSION);
  if (package_name == doc.end() || !package_name->value().is_string() || has_new_version == doc.end() ||
      !has_new_version->value().is_bool()) {
    return make_selfupdate_error(SUE_PackageInfoFormatError);
  }
  package_info.package_name = package_name->value().as_string();
  package_info.has_new_version = has_new_version->value().as_bool();
  if (!package_info.has_new_version)
    return {};

  auto package_version = doc.find(PACKAGEINFO_PACKAGE_VERSION);
  auto force_update = doc.find(PACKAGEINFO_FORCE_UPDATE);
  auto package_url = doc.find(PACKAGEINFO_PACKAGE_URL);
  auto package_size = doc.find(PACKAGEINFO_PACKAGE_SIZE);
  auto package_format = doc.find(PACKAGEINFO_PACKAGE_FORMAT);
  auto package_hash = doc.find(PACKAGEINFO_PACKAGE_HASH);
  auto update_title = doc.find(PACKAGEINFO_UPDATE_TITLE);
  auto update_description = doc.find(PACKAGEINFO_UPDATE_DESCRIPTION);
  if ((package_version == doc.end() || !package_version->value().is_string()) ||
      (force_update != doc.end() && !force_update->value().is_bool()) ||
      (package_url == doc.end() || !package_url->value().is_string()) ||
      (package_size == doc.end() || !package_size->value().is_int64()) ||
      (package_format == doc.end() || !package_format->value().is_string()) ||
      (package_hash == doc.end() || !package_hash->value().is_object()) ||
      (update_title == doc.end() || !update_title->value().is_string()) ||
      (update_description == doc.end() || !update_description->value().is_string())) {
    return make_selfupdate_error(SUE_PackageInfoFormatError);
  }

  package_info.package_version = package_version->value().as_string();
  if (force_update != doc.end()) {
    package_info.force_update = force_update->value().as_bool();
  }
  package_info.package_url = package_url->value().as_string();
  package_info.package_size = package_size->value().as_int64();
  if (package_format->value().as_string() == PACKAGEINFO_PACKAGE_FORMAT_ZIP) {
    package_info.package_format = package_format->value().as_string();
  } else {
    return make_selfupdate_error(SUE_UnsupportedPackageFormat);
  }
  auto package_hash_object = package_hash->value().as_object();
  for (auto it = package_hash_object.begin(); it != package_hash_object.end(); ++it) {
    if (!it->value().is_string()) {
      return make_selfupdate_error(SUE_PackageInfoFormatError);
    } else if (it->key() == PACKAGEINFO_PACKAGE_HASH_ALGO_MD5 || it->key() == PACKAGEINFO_PACKAGE_HASH_ALGO_SHA1 ||
               it->key() == PACKAGEINFO_PACKAGE_HASH_ALGO_SHA224 || it->key() == PACKAGEINFO_PACKAGE_HASH_ALGO_SHA256 ||
               it->key() == PACKAGEINFO_PACKAGE_HASH_ALGO_SHA384 || it->key() == PACKAGEINFO_PACKAGE_HASH_ALGO_SHA512) {
      package_info.package_hash.insert(std::make_pair(std::string(it->key()), std::string(it->value().as_string())));
    } else {
      return make_selfupdate_error(SUE_PackageInfoFormatError);
    }
  }
  package_info.update_title = update_title->value().as_string();
  package_info.update_description = update_description->value().as_string();
  return {};
}

} // namespace selfupdate
