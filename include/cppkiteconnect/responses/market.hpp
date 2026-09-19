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
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace cppkiteconnect {

using json = nlohmann::json;

/// Represents OHLC information of an instrument.
struct ohlc {
    double open = -1.0;
    double high = -1.0;
    double low = -1.0;
    double close = -1.0;
};

/// Represents market depth entry.
struct market_depth_entry {
    double price = -1.0;
    int quantity = -1;
    int orders = 0;
};

/// Represents market depth (buy/sell).
struct market_depth {
    std::vector<market_depth_entry> buy;
    std::vector<market_depth_entry> sell;
};

/// Represents quote information of an instrument.
struct quote {
    uint32_t instrument_token = 0;
    std::string timestamp;
    double last_price = -1.0;
    int last_quantity = -1;
    std::string last_trade_time;
    double average_price = -1.0;
    int64_t volume = -1;
    int buy_quantity = -1;
    int sell_quantity = -1;
    std::optional<ohlc> ohlc;
    double net_change = -1.0;
    double oi = -1.0;
    double oi_day_high = -1.0;
    double oi_day_low = -1.0;
    double lower_circuit_limit = -1.0;
    double upper_circuit_limit = -1.0;
    std::optional<market_depth> depth;
};

/// Represents OHLC quote (simplified OHLC data).
struct ohlc_quote {
    uint32_t instrument_token = 0;
    std::optional<ohlc> ohlc;
};

/// Represents an instrument.
struct instrument {
    uint32_t instrument_token = 0;
    std::string exchange;
    std::string tradingsymbol;
    std::string name;
    uint32_t lot_size = 0;
    int tick_size = 0;
    double strike = -1.0;
    std::string instrument_type;
    std::string expiry;
    uint32_t segment = 0;
};

// JSON serialization helpers
void from_json(const json& j, ohlc& o);
void from_json(const json& j, market_depth_entry& e);
void from_json(const json& j, market_depth& d);
void from_json(const json& j, quote& q);
void from_json(const json& j, ohlc_quote& oq);
void from_json(const json& j, instrument& i);

// Parser functions for client API
[[nodiscard]] quote_data parse_quote_data(json const& j);
[[nodiscard]] ohlc_data parse_ohlc_data(json const& j);
[[nodiscard]] std::vector<candle> parse_candles(json const& j);

} // namespace cppkiteconnect
