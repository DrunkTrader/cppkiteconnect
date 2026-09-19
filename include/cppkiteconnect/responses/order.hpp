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

/// Parameters required for placing an order.
struct place_order_params {
    int quantity = 0;
    std::optional<int> disclosed_quantity;
    std::optional<int> validity_ttl;
    std::optional<int> iceberg_legs;
    std::optional<int> iceberg_quantity;
    std::optional<double> price;
    std::optional<double> trigger_price;
    std::optional<double> square_off;
    std::optional<double> stop_loss;
    std::optional<double> trailing_stop_loss;
    std::string variety;
    std::string exchange;
    std::string symbol;
    std::string transaction_type;
    std::string product;
    std::string order_type;
    std::optional<std::string> validity;
    std::optional<std::string> tag;
};

/// Parameters required for modifying an order.
struct modify_order_params {
    std::optional<int> quantity;
    std::optional<int> disclosed_quantity;
    std::optional<double> price;
    std::optional<double> trigger_price;
    std::string variety;
    std::string order_id;
    std::optional<std::string> parent_order_id;
    std::optional<std::string> order_type;
    std::optional<std::string> validity;
};

/// Represents information of an order.
struct order {
    uint32_t instrument_token = 0;
    int quantity = -1;
    int disclosed_quantity = -1;
    int filled_quantity = -1;
    int pending_quantity = -1;
    int cancelled_quantity = -1;
    int validity_ttl = -1;
    double price = -1.0;
    double trigger_price = -1.0;
    double average_price = -1.0;
    
    std::string account_id;
    std::string placed_by;
    std::string order_id;
    std::string exchange_order_id;
    std::string parent_order_id;
    std::string status;
    std::string status_message;
    std::string order_timestamp;
    std::string exchange_update_timestamp;
    std::string exchange_timestamp;
    std::string rejected_by;
    std::string variety;
    std::string exchange;
    std::string tradingsymbol;
    std::string order_type;
    std::string transaction_type;
    std::string validity;
    std::string product;
};

/// Represents information of a trade.
struct trade {
    uint32_t instrument_token = 0;
    double average_price = -1.0;
    double quantity = -1.0;
    
    std::string trade_id;
    std::string product;
    std::string fill_timestamp;
    std::string exchange_timestamp;
    std::string exchange_order_id;
    std::string order_id;
    std::string transaction_type;
    std::string tradingsymbol;
    std::string exchange;
};

// JSON serialization helpers
void from_json(const json& j, place_order_params& p);
void from_json(const json& j, modify_order_params& p);
void from_json(const json& j, order& o);
void from_json(const json& j, trade& t);

// Parser functions for client API
[[nodiscard]] order parse_order_response(json const& j);
[[nodiscard]] std::vector<order> parse_orders_list(json const& j);

} // namespace cppkiteconnect
