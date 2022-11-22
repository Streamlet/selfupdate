#include <Windows.h>
#include <Winhttp.h>

#include "encoding.h"
#include "http_client.h"
#include "url.h"
#include <boost/scope_exit.hpp>
#include <charconv>
#include <memory>
#include <sstream>
#include <string_view>

namespace {

const unsigned HTTP_VERSION = 11;
const char *PROTOCOL_HTTPS  = "https";
const char *PROTOCOL_HTTP   = "http";

enum class HttpMethod : int {
  Get = 0,
  Post,
  Put,
  Delete,
};

struct HttpMethodString {
#ifdef _DEBUG
  HttpMethod method;
#endif
  const wchar_t *string;
};

#ifdef _DEBUG
#define _(m, s) m, s
#else
#define _(m, s) s
#endif

static const HttpMethodString HTTP_METHOD_STRING_TABLE[] = {
    {_(HttpMethod::Get, L"GET")},
    {_(HttpMethod::Post, L"POST")},
    {_(HttpMethod::Put, L"PUT")},
    {_(HttpMethod::Delete, L"DELETE")},
};

#undef _

} // namespace

class HttpClient::HttpSession {
public:
  HttpSession(std::string user_agent) {
    std::wstring ua = encoding::UTF8ToUCS2(user_agent);

    session_ = ::WinHttpOpen(ua.empty() ? L"WinHttp" : ua.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                             WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    DWORD options = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    ::WinHttpSetOption(session_, WINHTTP_OPTION_REDIRECT_POLICY, &options, sizeof(options));
  }
  ~HttpSession() {
    ::WinHttpCloseHandle(session_);
  }

  std::error_code SendAndReceive(HttpMethod method,
                                 const std::string_view &url_string,
                                 const RequestHeader &request_header,
                                 const std::string_view &request_body,
                                 unsigned *response_status,
                                 ResponseHeader *response_header,
                                 std::string *response_body,
                                 unsigned timeout = 0) {
    if (timeout > 0)
      ::WinHttpSetTimeouts(session_, timeout, timeout, timeout, timeout);

    Url url = Url::Parse(url_string);
    if (!url.valid)
      return std::make_error_code(std::errc::invalid_argument);
    if (url.protocol != PROTOCOL_HTTPS && url.protocol != PROTOCOL_HTTP)
      return std::make_error_code(std::errc::protocol_not_supported);
    bool ssl = url.protocol == PROTOCOL_HTTPS;

    std::wstring host  = encoding::UTF8ToUCS2(url.domain);
    std::wstring path  = encoding::UTF8ToUCS2(url.full_path);
    INTERNET_PORT port = INTERNET_DEFAULT_PORT;
    if (url.port.empty())
      port = ssl ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    else
      std::from_chars(url.port.data(), url.port.data() + url.port.length(), port);

    HINTERNET connection = Connect(session_, host, port);
    if (connection == nullptr) {
      return std::error_code(::GetLastError(), std::system_category());
    }
    BOOST_SCOPE_EXIT(connection) {
      ::WinHttpCloseHandle(connection);
    }
    BOOST_SCOPE_EXIT_END;

    HINTERNET request = OpenRequest(connection, ssl, method, path);
    if (request == nullptr) {
      return std::error_code(::GetLastError(), std::system_category());
    }

    BOOST_SCOPE_EXIT(request) {
      ::WinHttpCloseHandle(request);
    }
    BOOST_SCOPE_EXIT_END;

    if (!SendRequest(request, request_header, request_body)) {
      return std::error_code(::GetLastError(), std::system_category());
    }

    if (response_status != nullptr || response_header != nullptr) {
      if (!ReceiveHeader(request, response_status, response_header)) {
        return std::error_code(::GetLastError(), std::system_category());
      }
    }

    if (response_body != nullptr) {
      if (!ReceiveBody(request, response_body)) {
        return std::error_code(::GetLastError(), std::system_category());
      }
    }

    return {};
  }

private:
  HINTERNET Connect(HINTERNET session, const std::wstring &host, INTERNET_PORT port) {
    return ::WinHttpConnect(session, host.c_str(), port, 0);
  }

  HINTERNET OpenRequest(HINTERNET connection, bool ssl, HttpMethod method, const std::wstring &path) {
    const wchar_t *verb = HTTP_METHOD_STRING_TABLE[static_cast<int>(method)].string;
    return ::WinHttpOpenRequest(connection, verb, path.empty() ? L"/" : path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_REFRESH | (ssl ? WINHTTP_FLAG_SECURE : 0));
  }

  bool SendRequest(HINTERNET request, const RequestHeader &request_header, const std::string_view &request_body) {
    std::stringstream ss;
    for (const auto &h : request_header)
      ss << h.first << ": " << h.second << "\r\n";
    std::wstring header_string = encoding::UTF8ToUCS2(ss.str());
    if (!::WinHttpSendRequest(request, header_string.c_str(), header_string.length(),
                              const_cast<char *>(request_body.data()), request_body.length(), request_body.length(),
                              0)) {
      return false;
    }
    if (!::WinHttpReceiveResponse(request, nullptr)) {
      return false;
    }
    return true;
  }

  bool ReceiveHeader(HINTERNET request, unsigned *response_status, ResponseHeader *response_header) {
    if (response_status != nullptr) {
      DWORD status_code_size = 0;
      ::WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER,
                            &status_code_size, WINHTTP_NO_HEADER_INDEX);
      std::wstring status_code;
      status_code.resize(status_code_size * sizeof(wchar_t));
      ::WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX, status_code.data(),
                            &status_code_size, WINHTTP_NO_HEADER_INDEX);
      *response_status = _wtoi(status_code.c_str());
    }

    if (response_header != nullptr) {
      DWORD header_size = 0;
      ::WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER,
                            &header_size, WINHTTP_NO_HEADER_INDEX);
      std::wstring buffer_w;
      buffer_w.resize(header_size);
      if (!::WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS, WINHTTP_HEADER_NAME_BY_INDEX, buffer_w.data(),
                                 &header_size, WINHTTP_NO_HEADER_INDEX)) {
        return false;
      }
      std::string buffer = encoding::UCS2ToUTF8(buffer_w);

      for (size_t i = 0; i < buffer.length() && buffer[i] != '\0';) {
        char *k = buffer.data() + i;
        int len = strlen(k);
        char *v = strchr(k, ':');
        if (v != nullptr) {
          *v++ = '\0';
          if (*v == ' ')
            ++v;
          response_header->insert({k, v});
        }
        i += len + 1;
      }
    }

    return true;
  }

  bool ReceiveBody(HINTERNET request, std::string *response_body) {
    DWORD bytes_available = 0;
    std::string buffer;
    while (true) {
      if (!::WinHttpQueryDataAvailable(request, &bytes_available))
        return false;
      if (bytes_available == 0)
        break;
      while (bytes_available > 0) {
        buffer.resize(bytes_available);
        DWORD bytes_read = 0;
        if (!::WinHttpReadData(request, buffer.data(), bytes_available, &bytes_read))
          return false;
        if (bytes_read > 0) {
          response_body->append(buffer.data(), bytes_read);
        }
        bytes_available -= bytes_read;
      }
    }
    return true;
  }

private:
  HINTERNET session_;
};

HttpClient::HttpClient(std::string user_agent) : session_(std::make_unique<HttpSession>(std::move(user_agent))) {
}

HttpClient::~HttpClient() {
}

std::error_code HttpClient::Get(const std::string_view &url,
                                const RequestHeader &request_header,
                                unsigned *response_status,
                                ResponseHeader *response_header,
                                std::string *response_body,
                                unsigned timeout) {
  return session_->SendAndReceive(HttpMethod::Get, url, request_header, "", response_status, response_header,
                                  response_body);
}

std::error_code HttpClient::Post(const std::string_view &url,
                                 const RequestHeader &request_header,
                                 const std::string_view &request_body,
                                 unsigned *response_status,
                                 ResponseHeader *response_header,
                                 std::string *response_body,
                                 unsigned timeout) {
  return session_->SendAndReceive(HttpMethod::Post, url, request_header, request_body, response_status, response_header,
                                  response_body);
}

std::error_code HttpClient::Put(const std::string_view &url,
                                const RequestHeader &request_header,
                                const std::string_view &request_body,
                                unsigned *response_status,
                                ResponseHeader *response_header,
                                std::string *response_body,
                                unsigned timeout) {
  return session_->SendAndReceive(HttpMethod::Put, url, request_header, request_body, response_status, response_header,
                                  response_body);
}

std::error_code HttpClient::Delete(const std::string_view &url,
                                   const RequestHeader &request_header,
                                   unsigned *response_status,
                                   ResponseHeader *response_header,
                                   std::string *response_body,
                                   unsigned timeout) {
  return session_->SendAndReceive(HttpMethod::Delete, url, request_header, "", response_status, response_header,
                                  response_body);
}
