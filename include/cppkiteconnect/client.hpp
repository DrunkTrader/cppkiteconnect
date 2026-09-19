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

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "cppkiteconnect/types.hpp"
#include "cppkiteconnect/requests.hpp"

namespace cppkiteconnect {

// Forward declarations
class http_transport;
class websocket_transport;

/**
 * @brief Main client facade for Kite Connect API
 * 
 * Provides a modern, type-safe interface to all Kite Connect endpoints.
 * Thread-safe for concurrent read operations.
 */
class kite_client {
public:
    /**
     * @brief Construct a new kite client with configuration
     * @param config Client configuration
     */
    explicit kite_client(client_config config);
    
    /**
     * @brief Destructor - cleans up resources
     */
    ~kite_client();
    
    // Non-copyable, movable
    kite_client(kite_client const&) = delete;
    kite_client& operator=(kite_client const&) = delete;
    kite_client(kite_client&&) noexcept;
    kite_client& operator=(kite_client&&) noexcept;

    // ==================== Authentication ====================
    
    /**
     * @brief Get login URL for user authentication
     * @return Login URL to redirect user to
     */
    [[nodiscard]] std::string get_login_url() const;
    
    /**
     * @brief Generate session after user logs in
     * @param request_token Request token from callback
     * @param api_secret API secret
     * @return Access token
     * @throws authentication_error on failure
     */
    std::string generate_session(std::string const& request_token, std::string const& api_secret);
    
    /**
     * @brief Invalidate session (logout)
     */
    void invalidate_session();
    
    /**
     * @brief Check if client has valid access token
     */
    [[nodiscard]] bool is_authenticated() const;

    // ==================== User ====================
    
    /**
     * @brief Get user profile
     * @return User profile data
     */
    // TODO: Implement user profile response type
    
    /**
     * @brief Get user margins
     * @return Margin data
     */
    // TODO: Implement margin response type

    // ==================== Orders ====================
    
    /**
     * @brief Place an order
     * @param request Order parameters
     * @return Order ID
     * @throws order_rejected on failure
     */
    std::string place_order(order_request const& request);
    
    /**
     * @brief Modify an existing order
     * @param request Modification parameters
     * @return Order ID
     */
    std::string modify_order(modify_order_request const& request);
    
    /**
     * @brief Cancel an order
     * @param order_id Order ID to cancel
     * @param variety Order variety
     * @return Order ID
     */
    std::string cancel_order(std::string const& order_id, variety var = variety::REGULAR);
    
    /**
     * @brief Get list of orders
     * @return Vector of orders
     */
    // TODO: Implement order list response

    // ==================== Portfolio ====================
    
    /**
     * @brief Get holdings
     * @return Holdings data
     */
    // TODO: Implement holdings response
    
    /**
     * @brief Get positions
     * @return Positions data
     */
    // TODO: Implement positions response
    
    /**
     * @brief Convert position
     * @param request Conversion parameters
     */
    void convert_position(convert_position_request const& request);

    // ==================== Market Data ====================
    
    /**
     * @brief Get quotes for instruments
     * @param tokens List of instrument tokens
     * @return Quote data
     */
    // TODO: Implement quote response
    
    /**
     * @brief Get OHLC data for instruments
     * @param tokens List of instrument tokens
     */
    // TODO: Implement OHLC response
    
    /**
     * @brief Get historical candle data
     * @param instrument_token Instrument token
     * @param from_date Start date
     * @param to_date End date
     * @param interval Candle interval
     */
    // TODO: Implement historical data response
    
    /**
     * @brief Get all instruments
     * @return Vector of instruments
     */
    // TODO: Implement instrument list response
    
    /**
     * @brief Get instruments for given exchange
     * @param exchange Exchange name
     */
    // TODO: Implement exchange instruments response

    // ==================== GTT ====================
    
    /**
     * @brief Place GTT order
     * @todo Implement GTT functionality
     */
    
    /**
     * @brief Get GTT orders
     * @todo Implement GTT functionality
     */

    // ==================== Ticker ====================
    
    /**
     * @brief Get ticker client for WebSocket streaming
     * @return Reference to ticker instance
     */
    // TODO: Implement ticker return type
    // auto& get_ticker();

private:
    struct impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace cppkiteconnect
