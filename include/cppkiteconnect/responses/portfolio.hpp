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

/// Represents an individual holding.
struct holding {
    uint32_t instrument_token = 0;
    int quantity = -1;
    int t1_quantity = -1;
    int realised_quantity = -1;
    int collateral_quantity = -1;
    double price = -1.0;
    double average_price = -1.0;
    double last_price = -1.0;
    double close_price = -1.0;
    double pnl = -1.0;
    double day_change = -1.0;
    double day_change_percentage = -1.0;
    
    std::string tradingsymbol;
    std::string exchange;
    std::string isin;
    std::string product;
    std::string collateral_type;
};

/// Represents an individual position.
struct position {
    uint32_t instrument_token = 0;
    int quantity = -1;
    int buy_quantity = -1;
    int overnight_quantity = -1;
    int sell_quantity = -1;
    int day_buy_quantity = -1;
    int day_sell_quantity = -1;
    double multiplier = -1.0;
    double average_price = -1.0;
    double close_price = -1.0;
    double last_price = -1.0;
    double value = -1.0;
    double pnl = -1.0;
    double m2m = -1.0;
    double unrealised = -1.0;
    double realised = -1.0;
    double buy_price = -1.0;
    double buy_value = -1.0;
    double buy_m2m = -1.0;
    double sell_price = -1.0;
    double sell_value = -1.0;
    double sell_m2m = -1.0;
    double day_buy_price = -1.0;
    double day_buy_value = -1.0;
    double day_sell_price = -1.0;
    double day_sell_value = -1.0;
    
    std::string tradingsymbol;
    std::string exchange;
    std::string product;
};

/// Represents response of the get_positions() method.
struct positions_response {
    std::vector<position> net;
    std::vector<position> day;
};

/// Parameters required for the convert_position() method.
struct convert_position_params {
    int quantity = -1;
    std::string exchange;
    std::string symbol;
    std::string transaction_type;
    std::string position_type;
    std::string old_product;
    std::string new_product;
};

// JSON serialization helpers
void from_json(const json& j, holding& h);
void from_json(const json& j, position& p);
void from_json(const json& j, positions_response& pr);
void from_json(const json& j, convert_position_params& p);

// Parser functions for client API
[[nodiscard]] std::vector<holding> parse_holdings_list(json const& j);
[[nodiscard]] std::vector<position> parse_positions_list(json const& j);

} // namespace cppkiteconnect
