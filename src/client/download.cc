#include "../include/selfupdate/selfupdate.h"
#include "../utility/crypto.h"
#include "../utility/http_client.h"
#include "common.h"
#include <boost/scope_exit.hpp>
#include <cstdio>
#include <filesystem>
#include <sstream>

#ifdef _WIN32
#define ftell _ftelli64
#define fseek _fseeki64
#else
#define _FILE_OFFSET_BITS 64
#define ftell ftello
#define fseek fseeko
#endif

namespace selfupdate {

namespace {

long long ReadInteger(const std::filesystem::path &file) {
#ifdef _MSC_VER
  FILE *f = _wfopen(file.c_str(), L"r");
#else
  FILE *f = fopen(file.c_str(), "r");
#endif
  if (f == nullptr)
    return -1;
  unsigned long long v = 0;
  if (fscanf(f, "%lld", &v) != 1)
    return -2;
  fclose(f);
  return v;
}
void WriteInteger(const std::filesystem::path &file, long long v) {
#ifdef _MSC_VER
  FILE *f = _wfopen(file.c_str(), L"w");
#else
  FILE *f = fopen(file.c_str(), "w");
#endif
  if (f == nullptr)
    return;
  fprintf(f, "%lld", v);
  fclose(f);
}

const char *DOWNLOADING_FILE_SUFFIX = ".downloading";

bool VerifyPackage(const std::filesystem::path &package_file, const std::map<std::string, std::string> &hashes) {
  for (const auto &item : hashes) {
    std::string hash = item.second;
    std::transform(hash.begin(), hash.end(), hash.begin(), [](unsigned char c) {
      return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
    });
    if (item.first == PACKAGEINFO_PACKAGE_HASH_ALGO_MD5) {
      if (crypto::MD5File(package_file) != hash)
        return false;
    }
    if (item.first == PACKAGEINFO_PACKAGE_HASH_ALGO_SHA1) {
      if (crypto::SHA1File(package_file) != hash)
        return false;
    }
    if (item.first == PACKAGEINFO_PACKAGE_HASH_ALGO_SHA224) {
      if (crypto::SHA224File(package_file) != hash)
        return false;
    }
    if (item.first == PACKAGEINFO_PACKAGE_HASH_ALGO_SHA256) {
      if (crypto::SHA256File(package_file) != hash)
        return false;
    }
    if (item.first == PACKAGEINFO_PACKAGE_HASH_ALGO_SHA384) {
      if (crypto::SHA384File(package_file) != hash)
        return false;
    }
    if (item.first == PACKAGEINFO_PACKAGE_HASH_ALGO_SHA512) {
      if (crypto::SHA512File(package_file) != hash)
        return false;
    }
  }
  return true;
}

} // namespace

// If GCC optimize >= -O2, downloaded_size will be reset after http_client.Get.
// The below test "if (downloaded_size == total_size)" will never be satisfied.
// This mighe be a bug of GCC optimization, but I am not able to make a simple example.
#ifdef __GNUC__
#pragma GCC push_options
#pragma GCC optimize("O1")
#endif

std::error_code Download(const PackageInfo &package_info, DownloadProgressMonitor download_progress_monitor) {
  std::error_code ec;
  std::filesystem::path cache_dir = std::filesystem::temp_directory_path(ec);
  if (ec)
    return ec;

  cache_dir /= package_info.package_name;
  std::filesystem::create_directories(cache_dir, ec);
  if (ec)
    return ec;

  std::string package_file_name = package_info.package_name + PACKAGE_NAME_VERSION_SEP + package_info.package_version +
                                  FILE_NAME_EXT_SEP + package_info.package_format;
  std::filesystem::path package_file = cache_dir / package_file_name;
  std::filesystem::path package_downloading_file = cache_dir / (package_file_name + DOWNLOADING_FILE_SUFFIX);
  long long downloaded_size = ReadInteger(package_downloading_file);

  {
#ifdef _MSC_VER
    FILE *f = _wfopen(package_file.c_str(), L"wb");
#else
    FILE *f = fopen(package_file.c_str(), "wb");
#endif
    BOOST_SCOPE_EXIT(f) {
      fclose(f);
    }
    BOOST_SCOPE_EXIT_END
    fseek(f, 0, SEEK_END);
    long long offset = ftell(f);
    if (offset == package_info.package_size && downloaded_size < 0 &&
        VerifyPackage(package_file, package_info.package_hash)) {
      return {};
    }

    if (downloaded_size > 0 && offset > downloaded_size) {
      fseek(f, downloaded_size, SEEK_SET);
    } else {
      fseek(f, 0, SEEK_SET);
      downloaded_size = 0;
    }
    HttpClient http_client(SELFUPDATE_USER_AGENT);
    unsigned status = 0;
    HttpClient::ResponseHeader response_header;
    ec = http_client.Head(package_info.package_url, {}, &status, &response_header);
    if (ec)
      return ec;

    long long total_size = package_info.package_size;
    auto it = response_header.find("Content-Length");
    if (it != response_header.end()) {
      std::string content_length = it->second;
      total_size = atoll(content_length.c_str());
    }

    if (total_size != package_info.package_size)
      return make_selfupdate_error(SUE_PackageSizeError);

    std::stringstream range_expr;
    range_expr << "bytes=";
    range_expr << downloaded_size;
    range_expr << "-";
    HttpClient::RequestHeader request_header = {
        {"Range", range_expr.str()}
    };
    ec = http_client.Get(package_info.package_url, request_header, &status, nullptr,
                         [&](const void *data, size_t length) {
                           fwrite(data, 1, length, f);
                           fflush(f);
                           downloaded_size += length;
                           WriteInteger(package_downloading_file, downloaded_size);
                           if (download_progress_monitor != nullptr)
                             download_progress_monitor(downloaded_size, total_size);
                         });
    if (ec)
      return ec;
  }

  // if (downloaded_size != total_size) {
  //   return make_selfupdate_error(SUE_PackageSizeError);
  // }
  std::filesystem::remove(package_downloading_file);
  if (!VerifyPackage(package_file, package_info.package_hash)) {
    std::filesystem::remove(package_file);
    return make_selfupdate_error(SUE_PackageVerifyError);
  }
  return {};
}

#ifdef __GNUC__
#pragma GCC pop_options
#endif

} // namespace selfupdate
