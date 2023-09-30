#include "../include/selfupdate/selfupdate.h"
#include "../utility/http_client.h"
#include "../utility/log.h"
#include "common.h"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#ifndef _WIN32
#include <strings.h>
#define strnicmp strncasecmp
#endif

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

std::error_code Query(const std::string &query_url,
                      const std::multimap<std::string, std::string> &headers,
                      const std::string &query_body,
                      PackageInfo &package_info) {
  LOG_INFO("Querying: ", query_url, ", headers: ", headers.size(), ", body: ", query_body);
  HttpClient http_client(SELFUPDATE_USER_AGENT);
  unsigned status = 0;
  std::string response;
  std::error_code ec;
  if (query_body.empty()) {
    ec = http_client.Get(query_url, headers, &status, nullptr, &response, QUERY_TIMEOUT);
  } else {
    ec = http_client.Post(query_url, headers, query_body, &status, nullptr, &response, QUERY_TIMEOUT);
  }
  if (ec) {
    LOG_ERROR("Querying failed. Error category: ", ec.category().name(), ", code: ", ec.value(),
              ", message: ", ec.message());
    return ec;
  }
  if (status != 200) {
    LOG_ERROR("Querying failed. http status: ", status);
    return make_selfupdate_error(SUE_NetworkError);
  }
  LOG_INFO("Quering succeeded. Result: ", response);

  rapidjson::Document doc;
  doc.Parse<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag | rapidjson::kParseNanAndInfFlag>(
      response);
  if (doc.HasParseError()) {
    LOG_ERROR("Parsing json failed. Position: ", doc.GetErrorOffset(),
              ", error: ", rapidjson::GetParseError_En(doc.GetParseError()));
    return make_selfupdate_error(SUE_PackageInfoFormatError);
  }
  if (!doc.HasMember(PACKAGEINFO_PACKAGE_NAME) || !doc[PACKAGEINFO_PACKAGE_NAME].IsString() ||
      !doc.HasMember(PACKAGEINFO_HAS_NEW_VERSION) || !doc[PACKAGEINFO_HAS_NEW_VERSION].IsBool()) {
    LOG_ERROR("Package Info missing or type error: ", PACKAGEINFO_PACKAGE_NAME, PACKAGEINFO_HAS_NEW_VERSION);
    return make_selfupdate_error(SUE_PackageInfoFormatError);
  }
  package_info.package_name.assign(doc[PACKAGEINFO_PACKAGE_NAME].GetString(),
                                   doc[PACKAGEINFO_PACKAGE_NAME].GetStringLength());
  package_info.has_new_version = doc[PACKAGEINFO_HAS_NEW_VERSION].GetBool();
  if (!package_info.has_new_version) {
    LOG_INFO("No new version.");
    return {};
  }

  if (!doc.HasMember(PACKAGEINFO_PACKAGE_VERSION) || !doc[PACKAGEINFO_PACKAGE_VERSION].IsString() ||
      (doc.HasMember(PACKAGEINFO_FORCE_UPDATE) && !doc[PACKAGEINFO_FORCE_UPDATE].IsBool()) ||
      !doc.HasMember(PACKAGEINFO_PACKAGE_URL) || !doc[PACKAGEINFO_PACKAGE_URL].IsString() ||
      !doc.HasMember(PACKAGEINFO_PACKAGE_SIZE) || !doc[PACKAGEINFO_PACKAGE_SIZE].IsUint64() ||
      !doc.HasMember(PACKAGEINFO_PACKAGE_FORMAT) || !doc[PACKAGEINFO_PACKAGE_FORMAT].IsString() ||
      !doc.HasMember(PACKAGEINFO_PACKAGE_HASH) || !doc[PACKAGEINFO_PACKAGE_HASH].IsObject() ||
      !doc.HasMember(PACKAGEINFO_UPDATE_TITLE) || !doc[PACKAGEINFO_UPDATE_TITLE].IsString() ||
      !doc.HasMember(PACKAGEINFO_UPDATE_DESCRIPTION) || !doc[PACKAGEINFO_UPDATE_DESCRIPTION].IsString()) {
    LOG_ERROR("Package Info missing or type error: ", PACKAGEINFO_PACKAGE_VERSION, PACKAGEINFO_FORCE_UPDATE,
              PACKAGEINFO_PACKAGE_URL, PACKAGEINFO_PACKAGE_SIZE, PACKAGEINFO_PACKAGE_FORMAT, PACKAGEINFO_PACKAGE_HASH,
              PACKAGEINFO_UPDATE_TITLE, PACKAGEINFO_UPDATE_DESCRIPTION);
    return make_selfupdate_error(SUE_PackageInfoFormatError);
  }

  package_info.package_version.assign(doc[PACKAGEINFO_PACKAGE_VERSION].GetString(),
                                      doc[PACKAGEINFO_PACKAGE_VERSION].GetStringLength());
  if (doc.HasMember(PACKAGEINFO_FORCE_UPDATE)) {
    package_info.force_update = doc[PACKAGEINFO_FORCE_UPDATE].GetBool();
  }
  package_info.package_url.assign(doc[PACKAGEINFO_PACKAGE_URL].GetString(),
                                  doc[PACKAGEINFO_PACKAGE_URL].GetStringLength());
  package_info.package_size = doc[PACKAGEINFO_PACKAGE_SIZE].GetUint64();
  if (strnicmp(doc[PACKAGEINFO_PACKAGE_FORMAT].GetString(), PACKAGEINFO_PACKAGE_FORMAT_ZIP,
               doc[PACKAGEINFO_PACKAGE_FORMAT].GetStringLength()) == 0) {
    package_info.package_format.assign(doc[PACKAGEINFO_PACKAGE_FORMAT].GetString(),
                                       doc[PACKAGEINFO_PACKAGE_FORMAT].GetStringLength());
  } else {
    LOG_ERROR("Unsupported package format: ", std::string_view(doc[PACKAGEINFO_PACKAGE_FORMAT].GetString(),
                                                               doc[PACKAGEINFO_PACKAGE_FORMAT].GetStringLength()));
    return make_selfupdate_error(SUE_UnsupportedPackageFormat);
  }
  for (auto it = doc[PACKAGEINFO_PACKAGE_HASH].MemberBegin(); it != doc[PACKAGEINFO_PACKAGE_HASH].MemberEnd(); ++it) {
    if (!it->name.IsString() || !it->value.IsString()) {
      LOG_ERROR("Hash data error: ", PACKAGEINFO_PACKAGE_HASH);
      return make_selfupdate_error(SUE_PackageInfoFormatError);
    } else if (strnicmp(it->name.GetString(), PACKAGEINFO_PACKAGE_HASH_ALGO_MD5, it->name.GetStringLength()) == 0 ||
               strnicmp(it->name.GetString(), PACKAGEINFO_PACKAGE_HASH_ALGO_SHA1, it->name.GetStringLength()) == 0 ||
               strnicmp(it->name.GetString(), PACKAGEINFO_PACKAGE_HASH_ALGO_SHA224, it->name.GetStringLength()) == 0 ||
               strnicmp(it->name.GetString(), PACKAGEINFO_PACKAGE_HASH_ALGO_SHA256, it->name.GetStringLength()) == 0 ||
               strnicmp(it->name.GetString(), PACKAGEINFO_PACKAGE_HASH_ALGO_SHA384, it->name.GetStringLength()) == 0 ||
               strnicmp(it->name.GetString(), PACKAGEINFO_PACKAGE_HASH_ALGO_SHA512, it->name.GetStringLength()) == 0) {
      package_info.package_hash.insert(std::make_pair(std::string(it->name.GetString(), it->name.GetStringLength()),
                                                      std::string(it->value.GetString(), it->value.GetStringLength())));
    } else {
      LOG_ERROR("Unsupported hash algorithm: ", std::string_view(it->name.GetString(), it->name.GetStringLength()));
      return make_selfupdate_error(SUE_UnsupportedHashAlgorithm);
    }
  }
  package_info.update_title.assign(doc[PACKAGEINFO_UPDATE_TITLE].GetString(),
                                   doc[PACKAGEINFO_UPDATE_TITLE].GetStringLength());
  package_info.update_description.assign(doc[PACKAGEINFO_UPDATE_DESCRIPTION].GetString(),
                                         doc[PACKAGEINFO_UPDATE_DESCRIPTION].GetStringLength());
  LOG_INFO("New version found: ", package_info.package_version, ", url: ", package_info.package_url);
  return {};
}

} // namespace selfupdate
