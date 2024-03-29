#include <functional>
#include <map>
#include <memory>
#include <string_view>
#include <system_error>

class HttpClient {
public:
  HttpClient(std::string user_agent = DEFAULT_USER_AGENT);
  ~HttpClient();

  typedef std::multimap<std::string, std::string> RequestHeader;
  typedef std::multimap<std::string, std::string> ResponseHeader;
  typedef std::function<void(const void *data, size_t length)> ResponseBodyReceiver;

  std::error_code Head(const std::string_view &url,
                       const RequestHeader &request_header,
                       unsigned *response_status,
                       ResponseHeader *response_header,
                       unsigned timeout = 0);
  std::error_code Get(const std::string_view &url,
                      const RequestHeader &request_header,
                      unsigned *response_status,
                      ResponseHeader *response_header,
                      std::string *response_body,
                      unsigned timeout = 0);
  std::error_code Get(const std::string_view &url,
                      const RequestHeader &request_header,
                      unsigned *response_status,
                      ResponseHeader *response_header,
                      ResponseBodyReceiver response_body_receiver,
                      unsigned timeout = 0);
  std::error_code Post(const std::string_view &url,
                       const RequestHeader &request_header,
                       const std::string_view &request_body,
                       unsigned *response_status,
                       ResponseHeader *response_header,
                       std::string *response_body,
                       unsigned timeout = 0);
  std::error_code Post(const std::string_view &url,
                       const RequestHeader &request_header,
                       const std::string_view &request_body,
                       unsigned *response_status,
                       ResponseHeader *response_header,
                       ResponseBodyReceiver response_body_receiver,
                       unsigned timeout = 0);
  std::error_code Put(const std::string_view &url,
                      const RequestHeader &request_header,
                      const std::string_view &request_body,
                      unsigned *response_status,
                      ResponseHeader *response_header,
                      std::string *response_body,
                      unsigned timeout = 0);
  std::error_code Delete(const std::string_view &url,
                         const RequestHeader &request_header,
                         unsigned *response_status,
                         ResponseHeader *response_header,
                         std::string *response_body,
                         unsigned timeout = 0);

private:
  static const char *DEFAULT_USER_AGENT;
  class HttpSession;
  std::unique_ptr<HttpSession> session_;
};
