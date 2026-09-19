/*
 *  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 *  SPDX-License-Identifier: MIT
 *
 *  Copyright (c) 2020-2024 Bhumit Attarde.
 *
 *  Permission is hereby  granted, free of charge, to any  person obtaining a
 * copy of this software and associated  documentation files (the "Software"),
 * to deal in the Software  without restriction, including without  limitation
 * the rights to  use, copy,  modify, merge,  publish, distribute,  sublicense,
 * and/or  sell copies  of  the Software,  and  to  permit persons  to  whom the
 * Software  is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS
 * OR IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN
 * NO EVENT  SHALL THE AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY
 * CLAIM,  DAMAGES OR  OTHER LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <chrono>
#include <set>
#include <string>
#include <optional>

#include "cppkiteconnect/types.hpp"

namespace cppkiteconnect {

/**
 * @brief Error codes for all SDK operations
 */
enum class error_code {
    success = 0,
    network_failure,
    timeout,
    tls_failure,
    authentication_failed,
    authorization_failed,
    rate_limited,
    invalid_request,
    api_error,
    websocket_failure,
    protocol_error,
    parse_error,
    instrument_not_found,
    order_rejected
};

/**
 * @brief Detailed error information
 */
struct error_info {
    error_code code{error_code::success};
    int http_status{0};
    std::string message;
    std::string api_error_type;  ///< Kite-specific error type
    std::string request_id;       ///< For debugging/tracing
};

/**
 * @brief Base exception class for all Kite Connect errors
 */
class kite_exception : public std::runtime_error {
public:
    explicit kite_exception(error_info info);
    
    [[nodiscard]] error_info const& info() const noexcept;
    [[nodiscard]] error_code code() const noexcept;
    [[nodiscard]] int http_status() const noexcept;
    [[nodiscard]] std::string const& api_error_type() const noexcept;
    [[nodiscard]] std::string const& request_id() const noexcept;

private:
    error_info info_;
};

/**
 * @brief Authentication-related errors
 */
class authentication_error : public kite_exception {
public:
    explicit authentication_error(error_info info)
        : kite_exception(std::move(info)) {}
};

/**
 * @brief Authorization/permission errors
 */
class authorization_error : public kite_exception {
public:
    explicit authorization_error(error_info info)
        : kite_exception(std::move(info)) {}
};

/**
 * @brief Rate limit exceeded errors
 */
class rate_limit_error : public kite_exception {
public:
    explicit rate_limit_error(error_info info, std::chrono::seconds retry_after = std::chrono::seconds(0))
        : kite_exception(std::move(info)), retry_after_(retry_after) {}
    
    [[nodiscard]] std::chrono::seconds retry_after() const noexcept { return retry_after_; }

private:
    std::chrono::seconds retry_after_;
};

/**
 * @brief Network-related errors
 */
class network_error : public kite_exception {
public:
    explicit network_error(error_info info)
        : kite_exception(std::move(info)) {}
};

/**
 * @brief Timeout errors
 */
class timeout_error : public kite_exception {
public:
    explicit timeout_error(error_info info)
        : kite_exception(std::move(info)) {}
};

/**
 * @brief JSON parsing errors
 */
class parse_error : public kite_exception {
public:
    explicit parse_error(error_info info)
        : kite_exception(std::move(info)) {}
};

/**
 * @brief WebSocket-specific errors
 */
class websocket_error : public kite_exception {
public:
    explicit websocket_error(error_info info)
        : kite_exception(std::move(info)) {}
};

/**
 * @brief Binary protocol parsing errors
 */
class protocol_error : public kite_exception {
public:
    explicit protocol_error(error_info info)
        : kite_exception(std::move(info)) {}
};

/**
 * @brief Retry policy configuration
 */
struct retry_policy {
    std::size_t max_attempts{3};
    std::chrono::milliseconds initial_delay{100};
    std::chrono::milliseconds max_delay{5000};
    double backoff_multiplier{2.0};

    /// Error codes that should never be retried
    std::set<error_code> non_retryable{
        error_code::authentication_failed,
        error_code::authorization_failed,
        error_code::invalid_request,
        error_code::order_rejected
    };

    /**
     * @brief Check if an error code is retryable
     */
    [[nodiscard]] bool is_retryable(error_code code) const noexcept {
        return non_retryable.find(code) == non_retryable.end();
    }

    /**
     * @brief Calculate delay for a given attempt number
     */
    [[nodiscard]] std::chrono::milliseconds delay_for_attempt(std::size_t attempt) const {
        using namespace std::chrono;
        auto delay = initial_delay * static_cast<std::size_t>(std::pow(backoff_multiplier, attempt - 1));
        return std::min(delay, max_delay);
    }
};

/**
 * @brief Rate limiting policy configuration
 */
struct rate_limit_policy {
    std::size_t requests_per_second{3};  ///< Kite API default limit
    std::size_t burst_size{5};           ///< Allow brief bursts
};

/**
 * @brief Logging levels
 */
enum class log_level {
    trace,
    debug,
    info,
    warning,
    error,
    none
};

/**
 * @brief Client configuration
 */
struct client_config {
    /// API credentials
    std::string api_key;
    std::string access_token;  ///< Set after authentication

    /// Endpoints (configurable for sandbox/testing)
    std::string rest_endpoint{"https://api.kite.trade"};
    std::string websocket_endpoint{"wss://websocket.kite.trade"};

    /// Timeouts
    std::chrono::milliseconds connection_timeout{7000};
    std::chrono::milliseconds request_timeout{7000};

    /// Policies
    retry_policy retry;
    rate_limit_policy rate_limit;

    /// Optional: custom root CA for TLS
    std::string ca_cert_path;

    /// Logging configuration
    log_level logging{log_level::warning};
};

} // namespace cppkiteconnect
