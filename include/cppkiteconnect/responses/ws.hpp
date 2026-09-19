/*
 *  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 *  SPDX-License-Identifier: MIT
 *
 *  Copyright (c) 2020-2022 Bhumit Attarde
 *  Copyright (c) 2024 C++ Kite Connect Migration
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
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "cppkiteconnect/types.hpp"

namespace cppkiteconnect {

using json = nlohmann::json;

/// Represents a single entry in market depth returned by ticker.
struct depth_ws {
    int16_t orders = -1;
    int32_t quantity = -1;
    double price = -1.0;
};

/// OHLC data for WebSocket tick.
struct tick_ohlc {
    double open = -1.0;
    double high = -1.0;
    double low = -1.0;
    double close = -1.0;
};

/// Market depth for WebSocket tick.
struct tick_market_depth {
    std::vector<depth_ws> buy;
    std::vector<depth_ws> sell;
};

/// Represents a single market data tick from WebSocket.
struct tick {
    int32_t instrument_token = -1;
    int32_t timestamp = -1;
    int32_t last_trade_time = -1;
    int32_t last_traded_quantity = -1;
    int32_t total_buy_quantity = -1;
    int32_t total_sell_quantity = -1;
    int32_t volume_traded = -1;
    int32_t open_interest = -1;
    int32_t oi_day_high = -1;
    int32_t oi_day_low = -1;
    
    subscription_mode mode = subscription_mode::ltp;
    bool tradable = false;
    
    double last_price = -1.0;
    double average_trade_price = -1.0;
    double net_change = -1.0;
    
    tick_ohlc ohlc;
    std::vector<depth_ws> market_depth_buy;
    std::vector<depth_ws> market_depth_sell;
};

/// Represents a postback notification.
struct postback {
    int quantity = -1;
    int filled_quantity = -1;
    int unfilled_quantity = -1;
    double average_price = -1.0;
    double price = -1.0;
    double trigger_price = -1.0;
    
    std::string order_id;
    std::string exchange_order_id;
    std::string placed_by;
    std::string status;
    std::string status_message;
    std::string tradingsymbol;
    std::string exchange;
    std::string order_type;
    std::string transaction_type;
    std::string validity;
    std::string product;
    std::string user_id;
    std::string order_timestamp;
    std::string exchange_timestamp;
    std::string checksum;
};

// JSON serialization helpers
void from_json(const json& j, depth_ws& d);
void from_json(const json& j, tick_ohlc& o);
void from_json(const json& j, tick_market_depth& m);
void from_json(const json& j, tick& t);
void from_json(const json& j, postback& p);

} // namespace cppkiteconnect
