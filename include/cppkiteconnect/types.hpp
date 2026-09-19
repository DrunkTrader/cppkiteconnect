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

#include <string>
#include <string_view>

namespace cppkiteconnect {

/**
 * @brief Exchange constants for order placement
 */
enum class exchange {
    nse,   ///< National Stock Exchange
    bse,   ///< Bombay Stock Exchange
    nfo,   ///< NSE Futures & Options
    cde,   ///< NSE Currency Derivatives
    bfo,   ///< BSE Futures & Options
    mcx    ///< Multi Commodity Exchange
};

/**
 * @brief Transaction type (buy/sell)
 */
enum class transaction_type {
    buy,
    sell
};

/**
 * @brief Product types for positions
 */
enum class product {
    CNC,   ///< Cash and Carry (delivery)
    NRML,  ///< Normal (intraday/position)
    MIS,   ///< Margin Intraday Squareoff
    CO,    ///< Cover Order
    BO     ///< Bracket Order
};

/**
 * @brief Order types
 */
enum class order_type {
    MARKET, ///< Market order
    LIMIT,  ///< Limit order
    SL,     ///< Stop Loss Limit
    SLM     ///< Stop Loss Market
};

/**
 * @brief Order validity
 */
enum class validity {
    DAY,  ///< Valid for the day
    IOC,  ///< Immediate or Cancel
    TTL   ///< Time to Live (for BO/CO)
};

/**
 * @brief Order variety
 */
enum class variety {
    REGULAR, ///< Regular order
    BO,      ///< Bracket order
    CO,      ///< Cover order
    AMO      ///< After Market Order
};

/**
 * @brief Position type
 */
enum class position_type {
    net, ///< Net positions
    day  ///< Day positions
};

/**
 * @brief Ticker subscription mode
 */
enum class subscription_mode {
    quote, ///< Full quote data
    ltp,   ///< Last traded price only
    full   ///< Complete market depth
};

/**
 * @brief WebSocket connection state
 */
enum class connection_state {
    disconnected,
    connecting,
    connected,
    reconnecting,
    stopping
};

/**
 * @brief Convert exchange enum to string
 */
[[nodiscard]] std::string to_string(exchange e);

/**
 * @brief Convert string to exchange enum
 */
[[nodiscard]] exchange exchange_from_string(std::string_view s);

/**
 * @brief Convert transaction_type enum to string
 */
[[nodiscard]] std::string to_string(transaction_type t);

/**
 * @brief Convert string to transaction_type enum
 */
[[nodiscard]] transaction_type transaction_type_from_string(std::string_view s);

/**
 * @brief Convert product enum to string
 */
[[nodiscard]] std::string to_string(product p);

/**
 * @brief Convert string to product enum
 */
[[nodiscard]] product product_from_string(std::string_view s);

/**
 * @brief Convert order_type enum to string
 */
[[nodiscard]] std::string to_string(order_type o);

/**
 * @brief Convert string to order_type enum
 */
[[nodiscard]] order_type order_type_from_string(std::string_view s);

/**
 * @brief Convert validity enum to string
 */
[[nodiscard]] std::string to_string(validity v);

/**
 * @brief Convert string to validity enum
 */
[[nodiscard]] validity validity_from_string(std::string_view s);

/**
 * @brief Convert variety enum to string
 */
[[nodiscard]] std::string to_string(variety v);

/**
 * @brief Convert string to variety enum
 */
[[nodiscard]] variety variety_from_string(std::string_view s);

/**
 * @brief Convert position_type enum to string
 */
[[nodiscard]] std::string to_string(position_type p);

/**
 * @brief Convert string to position_type enum
 */
[[nodiscard]] position_type position_type_from_string(std::string_view s);

/**
 * @brief Convert subscription_mode enum to string
 */
[[nodiscard]] std::string to_string(subscription_mode m);

/**
 * @brief Convert string to subscription_mode enum
 */
[[nodiscard]] subscription_mode subscription_mode_from_string(std::string_view s);

/**
 * @brief Convert connection_state enum to string
 */
[[nodiscard]] std::string to_string(connection_state s);

/**
 * @brief Convert string to connection_state enum
 */
[[nodiscard]] connection_state connection_state_from_string(std::string_view s);

} // namespace cppkiteconnect
