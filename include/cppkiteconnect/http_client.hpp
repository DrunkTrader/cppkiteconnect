#pragma once

#include "errors.hpp"
#include "requests.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/json.hpp>
#include <string>
#include <optional>
#include <chrono>
#include <functional>

namespace cppkiteconnect {

namespace http {

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace ssl = boost::asio::ssl;
using error_code = boost::system::error_code;

/**
 * @brief Result of an HTTP request containing status, headers, and body.
 */
struct response_result {
    int status_code{};
    std::string body;
    std::map<std::string, std::string> headers;
    
    // Helper to parse JSON body (implementation in json_utils or here)
    [[nodiscard]] std::optional<boost::json::value> json() const;
};

/**
 * @brief Configuration for the HTTP client.
 */
struct client_config {
    std::string host = "api.kite.trade";
    int port = 443;
    std::chrono::seconds timeout{30};
    std::size_t max_body_size = 10 * 1024 * 1024; // 10MB
    bool verify_ssl = true;
    std::string user_agent = "cppkiteconnect/1.0";
};

/**
 * @brief Abstract interface for HTTP operations.
 * Allows mocking for unit tests.
 */
class i_http_client {
public:
    virtual ~i_http_client() = default;

    virtual boost::asio::awaitable<response_result> get(
        const std::string& path,
        const std::map<std::string, std::string>& headers,
        const std::map<std::string, std::string>& query_params) = 0;

    virtual boost::asio::awaitable<response_result> post(
        const std::string& path,
        const std::string& body,
        const std::map<std::string, std::string>& headers,
        const std::map<std::string, std::string>& query_params) = 0;

    virtual boost::asio::awaitable<response_result> put(
        const std::string& path,
        const std::string& body,
        const std::map<std::string, std::string>& headers,
        const std::map<std::string, std::string>& query_params) = 0;

    virtual boost::asio::awaitable<response_result> del(
        const std::string& path,
        const std::map<std::string, std::string>& headers,
        const std::map<std::string, std::string>& query_params) = 0;
};

/**
 * @brief Modern Boost.Beast based HTTP/SSL client.
 * 
 * Implements asynchronous HTTPS requests using C++20 coroutines.
 */
class beast_http_client : public i_http_client {
public:
    explicit beast_http_client(client_config config = {});
    ~beast_http_client() override = default;

    // Disable copy
    beast_http_client(const beast_http_client&) = delete;
    beast_http_client& operator=(const beast_http_client&) = delete;

    // Enable move
    beast_http_client(beast_http_client&&) = default;
    beast_http_client& operator=(beast_http_client&&) = default;

    boost::asio::awaitable<response_result> get(
        const std::string& path,
        const std::map<std::string, std::string>& headers,
        const std::map<std::string, std::string>& query_params) override;

    boost::asio::awaitable<response_result> post(
        const std::string& path,
        const std::string& body,
        const std::map<std::string, std::string>& headers,
        const std::map<std::string, std::string>& query_params) override;

    boost::asio::awaitable<response_result> put(
        const std::string& path,
        const std::string& body,
        const std::map<std::string, std::string>& headers,
        const std::map<std::string, std::string>& query_params) override;

    boost::asio::awaitable<response_result> del(
        const std::string& path,
        const std::map<std::string, std::string>& headers,
        const std::map<std::string, std::string>& query_params) override;

private:
    client_config config_;
    boost::asio::io_context io_context_;
    std::optional<ssl::context> ssl_ctx_;

    /**
     * @brief Core request implementation.
     */
    boost::asio::awaitable<response_result> execute_request(
        http::verb verb,
        const std::string& path,
        const std::string& body,
        std::map<std::string, std::string> headers,
        const std::map<std::string, std::string>& query_params);

    /**
     * @brief Build URL-encoded query string from map.
     */
    static std::string build_query_string(const std::map<std::string, std::string>& params);

    /**
     * @brief Initialize SSL context.
     */
    void init_ssl_context();
};

} // namespace http
} // namespace cppkiteconnect
