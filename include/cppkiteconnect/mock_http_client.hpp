#pragma once

#include "http_client.hpp"
#include <boost/asio/io_context.hpp>
#include <memory>
#include <string>
#include <map>
#include <optional>

namespace cppkiteconnect {

/**
 * @brief Mock HTTP client for unit testing.
 * 
 * Allows injecting predefined responses to test client logic without network calls.
 */
class mock_http_client : public http::i_http_client {
public:
    struct mock_response {
        int status_code{200};
        std::string body;
        std::map<std::string, std::string> headers;
    };

    mock_http_client() = default;
    ~mock_http_client() override = default;

    /**
     * @brief Set the response to return for the next request.
     */
    void set_next_response(mock_response response);

    /**
     * @brief Set a specific response for a given path and method.
     */
    void set_response_for(const std::string& method, const std::string& path, mock_response response);

    /**
     * @brief Set an exception to throw on the next request.
     */
    void set_next_exception(std::exception_ptr ex);

    // i_http_client interface
    boost::asio::awaitable<http::response_result> get(
        const std::string& path,
        const std::map<std::string, std::string>& headers,
        const std::map<std::string, std::string>& query_params) override;

    boost::asio::awaitable<http::response_result> post(
        const std::string& path,
        const std::string& body,
        const std::map<std::string, std::string>& headers,
        const std::map<std::string, std::string>& query_params) override;

    boost::asio::awaitable<http::response_result> put(
        const std::string& path,
        const std::string& body,
        const std::map<std::string, std::string>& headers,
        const std::map<std::string, std::string>& query_params) override;

    boost::asio::awaitable<http::response_result> del(
        const std::string& path,
        const std::map<std::string, std::string>& headers,
        const std::map<std::string, std::string>& query_params) override;

    /**
     * @brief Get the number of requests made.
     */
    [[nodiscard]] std::size_t request_count() const { return request_count_; }

    /**
     * @brief Get the last request details.
     */
    [[nodiscard]] const std::string& last_method() const { return last_method_; }
    [[nodiscard]] const std::string& last_path() const { return last_path_; }
    [[nodiscard]] const std::string& last_body() const { return last_body_; }

private:
    std::size_t request_count_{0};
    std::string last_method_;
    std::string last_path_;
    std::string last_body_;
    
    std::optional<mock_response> next_response_;
    std::map<std::string, mock_response> specific_responses_; // key: "METHOD:path"
    std::exception_ptr next_exception_;
};

} // namespace cppkiteconnect
