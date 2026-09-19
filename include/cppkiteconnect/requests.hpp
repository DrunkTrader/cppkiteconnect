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

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "cppkiteconnect/types.hpp"

namespace cppkiteconnect {

/**
 * @brief Order request structure for placing orders
 */
struct order_request {
    exchange exch{exchange::nse};
    std::string tradingsymbol;
    transaction_type txn_type{transaction_type::buy};
    product prod{product::CNC};
    order_type ord_type{order_type::MARKET};
    validity valid{validity::DAY};
    variety var{variety::REGULAR};

    int32_t quantity{};
    std::optional<int32_t> disclosed_quantity;
    std::optional<double> price;
    std::optional<double> trigger_price;
    std::optional<int32_t> squareoff;
    std::optional<double> stoploss;
    std::optional<double> trailing_stoploss;
    std::optional<std::string> tag;
    std::optional<int32_t> validity_ttl;
    std::optional<int32_t> iceberg_legs;
    std::optional<int32_t> iceberg_quantity;

    // Fluent interface for builder pattern
    order_request& set_exchange(exchange e) { exch = e; return *this; }
    order_request& set_tradingsymbol(std::string ts) { tradingsymbol = std::move(ts); return *this; }
    order_request& set_transaction_type(transaction_type t) { txn_type = t; return *this; }
    order_request& set_product(product p) { prod = p; return *this; }
    order_request& set_order_type(order_type o) { ord_type = o; return *this; }
    order_request& set_validity(validity v) { valid = v; return *this; }
    order_request& set_variety(variety v) { var = v; return *this; }
    order_request& set_quantity(int32_t q) { quantity = q; return *this; }
    order_request& set_disclosed_quantity(int32_t q) { disclosed_quantity = q; return *this; }
    order_request& set_price(double p) { price = p; return *this; }
    order_request& set_trigger_price(double p) { trigger_price = p; return *this; }
    order_request& set_squareoff(int32_t s) { squareoff = s; return *this; }
    order_request& set_stoploss(double s) { stoploss = s; return *this; }
    order_request& set_trailing_stoploss(double s) { trailing_stoploss = s; return *this; }
    order_request& set_tag(std::string t) { tag = std::move(t); return *this; }
};

/**
 * @brief Order modification request structure
 */
struct modify_order_request {
    std::string order_id;
    variety var{variety::REGULAR};
    std::optional<int32_t> quantity;
    std::optional<double> price;
    std::optional<order_type> ord_type;
    std::optional<validity> valid;
    std::optional<double> trigger_price;
    std::optional<int32_t> disclosed_quantity;
    std::optional<std::string> parent_order_id;

    order_request& set_order_id(std::string id) { order_id = std::move(id); return *this; }
    order_request& set_variety(variety v) { var = v; return *this; }
    order_request& set_quantity(int32_t q) { quantity = q; return *this; }
    order_request& set_price(double p) { price = p; return *this; }
    order_request& set_order_type(order_type o) { ord_type = o; return *this; }
    order_request& set_validity(validity v) { valid = v; return *this; }
    order_request& set_trigger_price(double p) { trigger_price = p; return *this; }
    order_request& set_disclosed_quantity(int32_t q) { disclosed_quantity = q; return *this; }
};

/**
 * @brief Position conversion request
 */
struct convert_position_request {
    exchange exch{exchange::nse};
    std::string tradingsymbol;
    position_type pos_type{position_type::net};
    product old_prod{product::CNC};
    product new_prod{product::NRML};
    transaction_type txn_type{transaction_type::buy};
    int32_t quantity{};
};

/**
 * @brief Instrument representation
 */
struct instrument {
    uint32_t token{};
    std::string symbol;
    std::string name;
    std::string exchange;
    std::string segment;
    std::string instrument_type;
    std::string expiry;
    double strike{};
    int32_t tick_size{};
    int32_t lot_size{};
};

/**
 * @brief Quote data structure
 */
struct quote {
    std::string instrument_token;
    std::string last_price_time;
    
    // OHLCV data
    struct ohlc {
        double open{};
        double high{};
        double low{};
        double close{};
    };
    
    ohlc ohlc_data;
    ohlc ohlc_5min;
    
    // Market depth
    struct market_depth_level {
        int32_t quantity{};
        int32_t orders{};
        double price{};
    };
    
    std::vector<market_depth_level> buy_depth;
    std::vector<market_depth_level> sell_depth;
    
    // Additional fields
    double last_price{};
    double average_price{};
    double net_change{};
    double lower_circuit_limit{};
    double upper_circuit_limit{};
    double open_interest{};
    int64_t volume{};
    int64_t total_buy_quantity{};
    int64_t total_sell_quantity{};
};

} // namespace cppkiteconnect
