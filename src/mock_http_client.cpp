#include "mock_http_client.hpp"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <stdexcept>

namespace cppkiteconnect {

void mock_http_client::set_next_response(mock_response response) {
    next_response_ = std::move(response);
}

void mock_http_client::set_response_for(const std::string& method, const std::string& path, mock_response response) {
    specific_responses_[method + ":" + path] = std::move(response);
}

void mock_http_client::set_next_exception(std::exception_ptr ex) {
    next_exception_ = ex;
}

boost::asio::awaitable<http::response_result> mock_http_client::get(
    const std::string& path,
    const std::map<std::string, std::string>& headers,
    const std::map<std::string, std::string>& query_params) {
    
    last_method_ = "GET";
    last_path_ = path;
    last_body_.clear();
    ++request_count_;

    if (next_exception_) {
        std::rethrow_exception(next_exception_);
    }

    auto key = "GET:" + path;
    if (specific_responses_.count(key)) {
        auto& resp = specific_responses_[key];
        http::response_result result{resp.status_code, resp.body, resp.headers};
        co_return result;
    }

    if (next_response_) {
        auto& resp = *next_response_;
        http::response_result result{resp.status_code, resp.body, resp.headers};
        next_response_.reset();
        co_return result;
    }

    // Default response
    co_return http::response_result{200, "{}", {}};
}

boost::asio::awaitable<http::response_result> mock_http_client::post(
    const std::string& path,
    const std::string& body,
    const std::map<std::string, std::string>& headers,
    const std::map<std::string, std::string>& query_params) {
    
    last_method_ = "POST";
    last_path_ = path;
    last_body_ = body;
    ++request_count_;

    if (next_exception_) {
        std::rethrow_exception(next_exception_);
    }

    auto key = "POST:" + path;
    if (specific_responses_.count(key)) {
        auto& resp = specific_responses_[key];
        http::response_result result{resp.status_code, resp.body, resp.headers};
        co_return result;
    }

    if (next_response_) {
        auto& resp = *next_response_;
        http::response_result result{resp.status_code, resp.body, resp.headers};
        next_response_.reset();
        co_return result;
    }

    co_return http::response_result{200, "{}", {}};
}

boost::asio::awaitable<http::response_result> mock_http_client::put(
    const std::string& path,
    const std::string& body,
    const std::map<std::string, std::string>& headers,
    const std::map<std::string, std::string>& query_params) {
    
    last_method_ = "PUT";
    last_path_ = path;
    last_body_ = body;
    ++request_count_;

    if (next_exception_) {
        std::rethrow_exception(next_exception_);
    }

    auto key = "PUT:" + path;
    if (specific_responses_.count(key)) {
        auto& resp = specific_responses_[key];
        http::response_result result{resp.status_code, resp.body, resp.headers};
        co_return result;
    }

    if (next_response_) {
        auto& resp = *next_response_;
        http::response_result result{resp.status_code, resp.body, resp.headers};
        next_response_.reset();
        co_return result;
    }

    co_return http::response_result{200, "{}", {}};
}

boost::asio::awaitable<http::response_result> mock_http_client::del(
    const std::string& path,
    const std::map<std::string, std::string>& headers,
    const std::map<std::string, std::string>& query_params) {
    
    last_method_ = "DELETE";
    last_path_ = path;
    last_body_.clear();
    ++request_count_;

    if (next_exception_) {
        std::rethrow_exception(next_exception_);
    }

    auto key = "DELETE:" + path;
    if (specific_responses_.count(key)) {
        auto& resp = specific_responses_[key];
        http::response_result result{resp.status_code, resp.body, resp.headers};
        co_return result;
    }

    if (next_response_) {
        auto& resp = *next_response_;
        http::response_result result{resp.status_code, resp.body, resp.headers};
        next_response_.reset();
        co_return result;
    }

    co_return http::response_result{200, "{}", {}};
}

} // namespace cppkiteconnect
