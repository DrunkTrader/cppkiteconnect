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

#include "cppkiteconnect/client.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include "cppkiteconnect/crypto.hpp"
#include "cppkiteconnect/errors.hpp"
#include "cppkiteconnect/http_client.hpp"
#include "cppkiteconnect/json_utils.hpp"
#include "cppkiteconnect/responses/market.hpp"
#include "cppkiteconnect/responses/order.hpp"
#include "cppkiteconnect/responses/portfolio.hpp"
#include "cppkiteconnect/responses/user.hpp"
#include "cppkiteconnect/ticker.hpp"
#include "cppkiteconnect/url_builder.hpp"

namespace cppkiteconnect {

// Internal implementation structure
struct kite_client::impl {
    client_config config;
    std::string access_token;
    std::unique_ptr<i_http_client> http_client;
    std::unique_ptr<ticker> ws_ticker;
    
    impl(client_config cfg) 
        : config(std::move(cfg))
        , http_client(std::make_unique<beast_http_client>(cfg.http_timeout, cfg.connect_timeout)) {
    }
    
    [[nodiscard]] url_builder build_endpoint(std::string_view endpoint) const {
        url_builder builder{config.base_url};
        builder.append_path(config.api_version);
        builder.append_path(endpoint);
        return builder;
    }
    
    void set_auth_header() {
        if (!access_token.empty()) {
            http_client->set_default_header("Authorization", 
                fmt::format("token {}:{}{}", config.api_key, config.api_secret, access_token));
        }
    }
};

// ==================== Construction/Destruction ====================

kite_client::kite_client(client_config config)
    : pimpl_(std::make_unique<impl>(std::move(config))) {
}

kite_client::~kite_client() = default;

kite_client::kite_client(kite_client&&) noexcept = default;
kite_client& kite_client::operator=(kite_client&&) noexcept = default;

// ==================== Authentication ====================

[[nodiscard]] std::string kite_client::get_login_url() const {
    url_builder builder{pimpl_->config.login_url};
    builder.append_param("api_key", pimpl_->config.api_key);
    return builder.build();
}

std::string kite_client::generate_session(std::string const& request_token, 
                                           std::string const& api_secret) {
    // Generate checksum using SHA-256
    std::string checksum_input = request_token + api_secret;
    sha256_hasher hasher;
    hasher.update(checksum_input);
    std::string checksum = hasher.finalize_hex();
    
    // Build request
    url_builder builder{pimpl_->build_endpoint("session/token")};
    
    nlohmann::json payload;
    payload["api_key"] = pimpl_->config.api_key;
    payload["request_token"] = request_token;
    payload["checksum"] = checksum;
    
    // Make POST request
    auto response = pimpl_->http_client->post(builder.build(), payload.dump());
    
    if (response.status_code != 200) {
        throw authentication_error(
            error_info{
                .code = "AUTH_ERROR",
                .message = fmt::format("Failed to generate session: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    // Parse response
    try {
        auto json_response = nlohmann::json::parse(response.body);
        auto auth_data = parse_auth_response(json_response);
        
        pimpl_->access_token = auth_data.access_token;
        pimpl_->set_auth_header();
        
        return pimpl_->access_token;
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse session response: {}", e.what())
            });
    }
}

void kite_client::invalidate_session() {
    if (!pimpl_->access_token.empty()) {
        url_builder builder{pimpl_->build_endpoint("session/token")};
        
        auto response = pimpl_->http_client->del(builder.build());
        
        // Even if DELETE fails, clear local token
        pimpl_->access_token.clear();
        pimpl_->http_client->remove_default_header("Authorization");
        
        if (response.status_code != 200) {
            // Log warning but don't throw - session is invalidated locally anyway
            fmt::print(stderr, "Warning: Server returned {} when invalidating session\n", 
                      response.status_code);
        }
    }
}

[[nodiscard]] bool kite_client::is_authenticated() const {
    return !pimpl_->access_token.empty();
}

// ==================== User ====================

user_profile kite_client::get_profile() {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint("user/profile")};
    auto response = pimpl_->http_client->get(builder.build());
    
    if (response.status_code != 200) {
        throw network_error(
            error_info{
                .code = "USER_ERROR",
                .message = fmt::format("Failed to get profile: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        return parse_user_profile(json_response);
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse profile response: {}", e.what())
            });
    }
}

margins kite_client::get_margins() {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint("user/margins")};
    auto response = pimpl_->http_client->get(builder.build());
    
    if (response.status_code != 200) {
        throw network_error(
            error_info{
                .code = "MARGIN_ERROR",
                .message = fmt::format("Failed to get margins: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        return parse_margins_response(json_response);
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse margins response: {}", e.what())
            });
    }
}

margins kite_client::get_margins(margin_segment segment) {
    pimpl_->set_auth_header();
    
    std::string segment_str = (segment == margin_segment::equity) ? "equity" : "commodity";
    url_builder builder{pimpl_->build_endpoint(fmt::format("user/margins/{}", segment_str))};
    auto response = pimpl_->http_client->get(builder.build());
    
    if (response.status_code != 200) {
        throw network_error(
            error_info{
                .code = "MARGIN_ERROR",
                .message = fmt::format("Failed to get {} margins: HTTP {}", 
                                      segment_str, response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        return parse_margins_response(json_response);
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse margins response: {}", e.what())
            });
    }
}

// ==================== Orders ====================

std::string kite_client::place_order(order_request const& request) {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint(
        fmt::format("orders/{}", to_string(request.variety)))};
    
    // Convert request to JSON payload
    nlohmann::json payload;
    payload["exchange"] = to_string(request.exchange);
    payload["tradingsymbol"] = request.tradingsymbol;
    payload["transaction_type"] = to_string(request.transaction_type);
    payload["quantity"] = request.quantity;
    payload["product"] = to_string(request.product);
    payload["order_type"] = to_string(request.order_type);
    
    if (request.price.has_value()) {
        payload["price"] = request.price.value();
    }
    if (request.validity.has_value()) {
        payload["validity"] = to_string(request.validity.value());
    }
    if (request.disclosed_quantity.has_value()) {
        payload["disclosed_quantity"] = request.disclosed_quantity.value();
    }
    if (request.trigger_price.has_value()) {
        payload["trigger_price"] = request.trigger_price.value();
    }
    if (request.squareoff.has_value()) {
        payload["squareoff"] = request.squareoff.value();
    }
    if (request.stoploss.has_value()) {
        payload["stoploss"] = request.stoploss.value();
    }
    if (request.trailing_stoploss.has_value()) {
        payload["trailing_stoploss"] = request.trailing_stoploss.value();
    }
    if (request.tag.has_value()) {
        payload["tag"] = request.tag.value();
    }
    
    auto response = pimpl_->http_client->post(builder.build(), payload.dump());
    
    if (response.status_code != 200) {
        throw protocol_error(
            error_info{
                .code = "ORDER_REJECTED",
                .message = fmt::format("Order rejected: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        auto result = parse_order_response(json_response);
        return result.order_id;
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse order response: {}", e.what())
            });
    }
}

std::string kite_client::modify_order(modify_order_request const& request) {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint(
        fmt::format("orders/{}/{}", to_string(request.variety), request.order_id))};
    
    nlohmann::json payload;
    payload["exchange"] = to_string(request.exchange);
    payload["tradingsymbol"] = request.tradingsymbol;
    payload["transaction_type"] = to_string(request.transaction_type);
    
    if (request.quantity.has_value()) {
        payload["quantity"] = request.quantity.value();
    }
    if (request.price.has_value()) {
        payload["price"] = request.price.value();
    }
    if (request.order_type.has_value()) {
        payload["order_type"] = to_string(request.order_type.value());
    }
    if (request.trigger_price.has_value()) {
        payload["trigger_price"] = request.trigger_price.value();
    }
    if (request.validity.has_value()) {
        payload["validity"] = to_string(request.validity.value());
    }
    if (request.disclosed_quantity.has_value()) {
        payload["disclosed_quantity"] = request.disclosed_quantity.value();
    }
    
    auto response = pimpl_->http_client->put(builder.build(), payload.dump());
    
    if (response.status_code != 200) {
        throw protocol_error(
            error_info{
                .code = "MODIFY_FAILED",
                .message = fmt::format("Modify failed: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        auto result = parse_order_response(json_response);
        return result.order_id;
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse modify response: {}", e.what())
            });
    }
}

std::string kite_client::cancel_order(std::string const& order_id, variety var) {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint(
        fmt::format("orders/{}/{}", to_string(var), order_id))};
    
    auto response = pimpl_->http_client->del(builder.build());
    
    if (response.status_code != 200) {
        throw protocol_error(
            error_info{
                .code = "CANCEL_FAILED",
                .message = fmt::format("Cancel failed: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        auto result = parse_order_response(json_response);
        return result.order_id;
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse cancel response: {}", e.what())
            });
    }
}

std::vector<order> kite_client::get_orders() {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint("orders")};
    auto response = pimpl_->http_client->get(builder.build());
    
    if (response.status_code != 200) {
        throw network_error(
            error_info{
                .code = "ORDERS_ERROR",
                .message = fmt::format("Failed to get orders: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        return parse_orders_list(json_response);
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse orders list: {}", e.what())
            });
    }
}

// ==================== Portfolio ====================

std::vector<holding> kite_client::get_holdings() {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint("portfolio/holdings")};
    auto response = pimpl_->http_client->get(builder.build());
    
    if (response.status_code != 200) {
        throw network_error(
            error_info{
                .code = "HOLDINGS_ERROR",
                .message = fmt::format("Failed to get holdings: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        return parse_holdings_list(json_response);
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse holdings: {}", e.what())
            });
    }
}

std::vector<position> kite_client::get_positions() {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint("portfolio/positions")};
    auto response = pimpl_->http_client->get(builder.build());
    
    if (response.status_code != 200) {
        throw network_error(
            error_info{
                .code = "POSITIONS_ERROR",
                .message = fmt::format("Failed to get positions: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        return parse_positions_list(json_response);
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse positions: {}", e.what())
            });
    }
}

void kite_client::convert_position(convert_position_request const& request) {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint("positions")};
    
    nlohmann::json payload;
    payload["exchange"] = to_string(request.exchange);
    payload["tradingsymbol"] = request.tradingsymbol;
    payload["transaction_type"] = to_string(request.transaction_type);
    payload["position_type"] = to_string(request.position_type);
    payload["quantity"] = request.quantity;
    payload["old_product"] = to_string(request.old_product);
    payload["new_product"] = to_string(request.new_product);
    
    auto response = pimpl_->http_client->put(builder.build(), payload.dump());
    
    if (response.status_code != 200) {
        throw protocol_error(
            error_info{
                .code = "CONVERT_FAILED",
                .message = fmt::format("Convert failed: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
}

// ==================== Market Data ====================

quote_data kite_client::get_quotes(std::vector<uint32_t> const& tokens) {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint("quotes")};
    for (auto token : tokens) {
        builder.append_param("i", std::to_string(token));
    }
    
    auto response = pimpl_->http_client->get(builder.build());
    
    if (response.status_code != 200) {
        throw network_error(
            error_info{
                .code = "QUOTES_ERROR",
                .message = fmt::format("Failed to get quotes: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        return parse_quote_data(json_response);
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse quotes: {}", e.what())
            });
    }
}

ohlc_data kite_client::get_ohlc(std::vector<uint32_t> const& tokens) {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint("ohlc")};
    for (auto token : tokens) {
        builder.append_param("i", std::to_string(token));
    }
    
    auto response = pimpl_->http_client->get(builder.build());
    
    if (response.status_code != 200) {
        throw network_error(
            error_info{
                .code = "OHLC_ERROR",
                .message = fmt::format("Failed to get OHLC: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        return parse_ohlc_data(json_response);
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse OHLC: {}", e.what())
            });
    }
}

std::vector<candle> kite_client::get_historical_data(
    uint32_t instrument_token,
    std::chrono::system_clock::time_point from_date,
    std::chrono::system_clock::time_point to_date,
    candle_interval interval) {
    
    pimpl_->set_auth_header();
    
    // Format dates as YYYY-MM-DD
    auto from_time = std::chrono::system_clock::to_time_t(from_date);
    auto to_time = std::chrono::system_clock::to_time_t(to_date);
    
    char from_buf[11], to_buf[11];
    std::strftime(from_buf, sizeof(from_buf), "%Y-%m-%d", std::localtime(&from_time));
    std::strftime(to_buf, sizeof(to_buf), "%Y-%m-%d", std::localtime(&to_time));
    
    url_builder builder{pimpl_->build_endpoint(
        fmt::format("instruments/historical/{}/{}", instrument_token, to_string(interval)))};
    builder.append_param("from", from_buf);
    builder.append_param("to", to_buf);
    
    auto response = pimpl_->http_client->get(builder.build());
    
    if (response.status_code != 200) {
        throw network_error(
            error_info{
                .code = "HISTORICAL_ERROR",
                .message = fmt::format("Failed to get historical data: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        return parse_candles(json_response);
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse historical data: {}", e.what())
            });
    }
}

std::vector<instrument> kite_client::get_all_instruments() {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint("instruments/all")};
    auto response = pimpl_->http_client->get(builder.build());
    
    if (response.status_code != 200) {
        throw network_error(
            error_info{
                .code = "INSTRUMENTS_ERROR",
                .message = fmt::format("Failed to get instruments: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    // CSV response for instruments
    return parse_instruments(response.body);
}

std::vector<instrument> kite_client::get_instruments_by_exchange(exchange exch) {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint(
        fmt::format("instruments/{}", to_string(exch)))};
    auto response = pimpl_->http_client->get(builder.build());
    
    if (response.status_code != 200) {
        throw network_error(
            error_info{
                .code = "INSTRUMENTS_ERROR",
                .message = fmt::format("Failed to get instruments: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    // CSV response for instruments
    return parse_instruments(response.body);
}

// ==================== GTT ====================

gtt_trigger kite_client::place_gtt(gtt_request const& request) {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint("gtt/triggers")};
    
    nlohmann::json payload;
    payload["tradingsymbol"] = request.tradingsymbol;
    payload["exchange"] = to_string(request.exchange);
    payload["transaction_type"] = to_string(request.transaction_type);
    payload["trigger_type"] = to_string(request.trigger_type);
    
    // Handle single or multiple trigger values
    if (request.trigger_values.size() == 1) {
        payload["trigger_values"] = request.trigger_values[0];
    } else {
        nlohmann::json triggers = nlohmann::json::array();
        for (auto val : request.trigger_values) {
            triggers.push_back(val);
        }
        payload["trigger_values"] = triggers;
    }
    
    payload["quantity"] = request.quantity;
    
    if (request.price.has_value()) {
        payload["price"] = request.price.value();
    }
    if (request.order_type.has_value()) {
        payload["order_type"] = to_string(request.order_type.value());
    }
    if (request.product.has_value()) {
        payload["product"] = to_string(request.product.value());
    }
    
    auto response = pimpl_->http_client->post(builder.build(), payload.dump());
    
    if (response.status_code != 200) {
        throw protocol_error(
            error_info{
                .code = "GTT_ERROR",
                .message = fmt::format("Failed to place GTT: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        return parse_gtt_trigger(json_response);
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse GTT response: {}", e.what())
            });
    }
}

std::vector<gtt_trigger> kite_client::get_gtt_triggers() {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint("gtt/triggers")};
    auto response = pimpl_->http_client->get(builder.build());
    
    if (response.status_code != 200) {
        throw network_error(
            error_info{
                .code = "GTT_ERROR",
                .message = fmt::format("Failed to get GTT triggers: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        return parse_gtt_triggers_list(json_response);
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse GTT list: {}", e.what())
            });
    }
}

// ==================== MF ====================

std::vector<mf_order> kite_client::get_mf_orders() {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint("mf/orders")};
    auto response = pimpl_->http_client->get(builder.build());
    
    if (response.status_code != 200) {
        throw network_error(
            error_info{
                .code = "MF_ORDER_ERROR",
                .message = fmt::format("Failed to get MF orders: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        return parse_mf_orders_list(json_response);
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse MF orders: {}", e.what())
            });
    }
}

std::vector<mf_holding> kite_client::get_mf_holdings() {
    pimpl_->set_auth_header();
    
    url_builder builder{pimpl_->build_endpoint("mf/holdings")};
    auto response = pimpl_->http_client->get(builder.build());
    
    if (response.status_code != 200) {
        throw network_error(
            error_info{
                .code = "MF_HOLDING_ERROR",
                .message = fmt::format("Failed to get MF holdings: HTTP {}", response.status_code),
                .http_status = response.status_code
            });
    }
    
    try {
        auto json_response = nlohmann::json::parse(response.body);
        return parse_mf_holdings_list(json_response);
    } catch (nlohmann::json::exception const& e) {
        throw parse_error(
            error_info{
                .code = "PARSE_ERROR",
                .message = fmt::format("Failed to parse MF holdings: {}", e.what())
            });
    }
}

// ==================== Ticker ====================

ticker& kite_client::get_ticker() {
    if (!pimpl_->ws_ticker) {
        ticker_config ws_config;
        ws_config.api_key = pimpl_->config.api_key;
        ws_config.access_token = pimpl_->access_token;
        ws_config.root_url = pimpl_->config.ws_root_url;
        ws_config.reconnect_enabled = true;
        ws_config.max_reconnect_attempts = 50;
        ws_config.reconnect_delay = std::chrono::seconds(2);
        
        pimpl_->ws_ticker = std::make_unique<ticker>(ws_config);
    }
    return *pimpl_->ws_ticker;
}

} // namespace cppkiteconnect
