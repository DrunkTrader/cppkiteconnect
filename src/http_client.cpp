#include "http_client.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <sstream>
#include <stdexcept>

namespace cppkiteconnect {
namespace http {

namespace beast = boost::beast;
using tcp = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
using error_code = boost::system::error_code;

// response_result implementation
[[nodiscard]] std::optional<boost::json::value> response_result::json() const {
    try {
        if (body.empty()) {
            return std::nullopt;
        }
        return boost::json::parse(body);
    } catch (...) {
        return std::nullopt;
    }
}

// beast_http_client implementation
beast_http_client::beast_http_client(client_config config)
    : config_(std::move(config)) {
    init_ssl_context();
}

void beast_http_client::init_ssl_context() {
    ssl_ctx_.emplace(ssl::context::tls_client);
    
    if (config_.verify_ssl) {
        ssl_ctx_->set_verify_mode(ssl::verify_peer);
        // In production, load system CA certificates
        // ssl_ctx_->set_default_verify_paths();
    } else {
        ssl_ctx_->set_verify_mode(ssl::verify_none);
    }
}

std::string beast_http_client::build_query_string(const std::map<std::string, std::string>& params) {
    if (params.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    bool first = true;
    for (const auto& [key, value] : params) {
        if (!first) {
            oss << "&";
        }
        // In production, properly URL-encode key and value
        oss << key << "=" << value;
        first = false;
    }
    return "?" + oss.str();
}

boost::asio::awaitable<response_result> beast_http_client::execute_request(
    http::verb verb,
    const std::string& path,
    const std::string& body,
    std::map<std::string, std::string> headers,
    const std::map<std::string, std::string>& query_params) {
    
    using namespace boost::asio::experimental::awaitable_operators;
    
    auto executor = co_await boost::asio::this_coro::executor;
    
    // Build request target
    std::string target = path + build_query_string(query_params);
    
    // Create SSL stream
    tcp::resolver resolver(executor);
    ssl::stream<tcp::socket> stream{executor, *ssl_ctx_};
    
    // Set timeout
    beast::get_lowest_layer(stream).expires_after(config_.timeout);
    
    // Resolve host
    auto results = co_await resolver.async_resolve(
        config_.host,
        std::to_string(config_.port),
        boost::asio::use_awaitable);
    
    // Connect
    co_await beast::get_lowest_layer(stream).async_connect(
        results,
        boost::asio::use_awaitable);
    
    // SSL handshake
    co_await stream.async_handshake(
        ssl::stream_base::client,
        boost::asio::use_awaitable);
    
    // Build request
    http::request<http::string_body> req{verb, target, 11};
    req.set(http::field::host, config_.host);
    req.set(http::field::user_agent, config_.user_agent);
    req.set(http::field::accept, "application/json");
    req.set(http::field::content_type, "application/json");
    
    for (const auto& [key, value] : headers) {
        req.set(key, value);
    }
    
    if (!body.empty() && (verb == http::verb::post || verb == http::verb::put)) {
        req.body() = body;
        req.prepare_payload();
    }
    
    // Send request
    co_await http::async_write(stream, req, boost::asio::use_awaitable);
    
    // Read response
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    co_await http::async_read(stream, buffer, res, boost::asio::use_awaitable);
    
    // Graceful shutdown
    error_code ec;
    co_await stream.async_shutdown(ec);
    
    // Build result
    response_result result;
    result.status_code = res.result_int();
    result.body = std::move(res.body());
    
    for (const auto& field : res.base()) {
        result.headers[std::string(field.name_string())] = std::string(field.value());
    }
    
    // Check for HTTP errors
    if (result.status_code >= 400) {
        throw protocol_error("HTTP error: " + std::to_string(result.status_code), result.status_code);
    }
    
    co_return result;
}

boost::asio::awaitable<response_result> beast_http_client::get(
    const std::string& path,
    const std::map<std::string, std::string>& headers,
    const std::map<std::string, std::string>& query_params) {
    co_return co_await execute_request(http::verb::get, path, "", headers, query_params);
}

boost::asio::awaitable<response_result> beast_http_client::post(
    const std::string& path,
    const std::string& body,
    const std::map<std::string, std::string>& headers,
    const std::map<std::string, std::string>& query_params) {
    co_return co_await execute_request(http::verb::post, path, body, headers, query_params);
}

boost::asio::awaitable<response_result> beast_http_client::put(
    const std::string& path,
    const std::string& body,
    const std::map<std::string, std::string>& headers,
    const std::map<std::string, std::string>& query_params) {
    co_return co_await execute_request(http::verb::put, path, body, headers, query_params);
}

boost::asio::awaitable<response_result> beast_http_client::del(
    const std::string& path,
    const std::map<std::string, std::string>& headers,
    const std::map<std::string, std::string>& query_params) {
    co_return co_await execute_request(http::verb::delete_, path, "", headers, query_params);
}

} // namespace http
} // namespace cppkiteconnect
