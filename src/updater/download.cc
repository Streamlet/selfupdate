#include "../common.h"
#include <cstdio>
#include <filesystem>
#include <selfupdate/updater.h>
#include <sstream>
#include <xl/crypto>
#include <xl/http>
#include <xl/log>
#include <xl/scope_exit>

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
  if (f == nullptr) {
    return -1;
  }
  unsigned long long v = 0;
  if (fscanf(f, "%lld", &v) != 1) {
    return -2;
  }
  fclose(f);
  return v;
}
void WriteInteger(const std::filesystem::path &file, long long v) {
#ifdef _MSC_VER
  FILE *f = _wfopen(file.c_str(), L"w");
#else
  FILE *f = fopen(file.c_str(), "w");
#endif
  if (f == nullptr) {
    return;
  }
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
      if (xl::crypto::md5_file(package_file.c_str()) != hash) {
        return false;
      }
    }
    if (item.first == PACKAGEINFO_PACKAGE_HASH_ALGO_SHA1) {
      if (xl::crypto::sha1_file(package_file.c_str()) != hash) {
        return false;
      }
    }
    if (item.first == PACKAGEINFO_PACKAGE_HASH_ALGO_SHA224) {
      if (xl::crypto::sha224_file(package_file.c_str()) != hash) {
        return false;
      }
    }
    if (item.first == PACKAGEINFO_PACKAGE_HASH_ALGO_SHA256) {
      if (xl::crypto::sha256_file(package_file.c_str()) != hash) {
        return false;
      }
    }
    if (item.first == PACKAGEINFO_PACKAGE_HASH_ALGO_SHA384) {
      if (xl::crypto::sha384_file(package_file.c_str()) != hash) {
        return false;
      }
    }
    if (item.first == PACKAGEINFO_PACKAGE_HASH_ALGO_SHA512) {
      if (xl::crypto::sha512_file(package_file.c_str()) != hash) {
        return false;
      }
    }
  }
  return true;
}

} // namespace

bool Download(const PackageInfo &package_info, DownloadProgressMonitor download_progress_monitor) {
  XL_LOG_INFO("Downloanding: ", package_info.package_url);

  std::error_code ec;
  std::filesystem::path cache_dir = std::filesystem::temp_directory_path(ec);
  if (ec) {
    XL_LOG_ERROR("Get temp dir error. Error category: ", ec.category().name(), ", code: ", ec.value(),
                 ", message: ", ec.message());
    return false;
  }

  cache_dir /= package_info.package_name;
  std::filesystem::create_directories(cache_dir, ec);
  if (ec) {
    XL_LOG_ERROR("Create cache dir error. dir: ", cache_dir.u8string(), ", error category: ", ec.category().name(),
                 ", code: ", ec.value(), ", message: ", ec.message());
    return false;
  }

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
    if (f == NULL) {
      XL_LOG_ERROR("Open local file error: ", package_file);
      return false;
    }
    XL_ON_BLOCK_EXIT(fclose, f);
    fseek(f, 0, SEEK_END);
    long long offset = ftell(f);
    if (offset == package_info.package_size && downloaded_size < 0 &&
        VerifyPackage(package_file, package_info.package_hash)) {
      XL_LOG_INFO("Package file already downloaded and verified OK: ", package_file);
      return {};
    }

    if (downloaded_size > 0 && offset > downloaded_size) {
      fseek(f, downloaded_size, SEEK_SET);
    } else {
      fseek(f, 0, SEEK_SET);
      downloaded_size = 0;
    }
    xl::http::Request request;
    request.url = package_info.package_url;
    request.method = xl::http::METHOD_HEAD;
    xl::http::Response response;
    xl::http::Headers response_headers;
    response.headers = &response_headers;
    xl::http::Option option;
    option.user_agent = SELFUPDATE_USER_AGENT;
    int status = xl::http::send(request, &response);
    if (status != 200) {
      XL_LOG_ERROR("Request HEAD error: ", package_info.package_url, ", http status/error: ", status);
      return false;
    }

    long long total_size = package_info.package_size;
    auto it = response_headers.find("Content-Length");
    if (it != response_headers.end()) {
      std::string content_length = it->second;
      total_size = atoll(content_length.c_str());
    }

    if (total_size != package_info.package_size) {
      XL_LOG_ERROR("Package size error, expected: ", package_info.package_size, ", got: ", total_size);
      return false;
    }

    std::stringstream range_expr;
    range_expr << "bytes=";
    range_expr << downloaded_size;
    range_expr << "-";
    request.headers = {
        {"Range", range_expr.str()}
    };
    response_headers.clear();
    status = xl::http::get(package_info.package_url, request.headers, response_headers,
                           [&](const void *buffer, size_t size) -> size_t {
                             fwrite(buffer, 1, size, f);
                             fflush(f);
                             downloaded_size += size;
                             WriteInteger(package_downloading_file, downloaded_size);
                             if (download_progress_monitor != nullptr) {
                               download_progress_monitor(downloaded_size, total_size);
                             }
                             return size;
                           });
    if (status != 200) {
      XL_LOG_ERROR("Download package error: ", package_info.package_url, ", status/error: ", status);
      return false;
    }
    if (status != 200) {
      XL_LOG_ERROR("Querying failed. http status: ", status);
      return false;
    }
  }

  // if (downloaded_size != total_size) {
  //   return make_selfupdate_error(SUE_PackageSizeError);
  // }
  std::filesystem::remove(package_downloading_file);
  if (!VerifyPackage(package_file, package_info.package_hash)) {
    std::filesystem::remove(package_file);
    XL_LOG_ERROR("Verify package error: ", package_file);
    return false;
  }

  XL_LOG_INFO("Downloaded package OK: ", package_file);
  return true;
}

} // namespace selfupdate
