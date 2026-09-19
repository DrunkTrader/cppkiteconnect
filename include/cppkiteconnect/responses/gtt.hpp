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

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "order.hpp"

namespace cppkiteconnect {

using json = nlohmann::json;

/// Represents parameters for GTT order legs.
struct gtt_order_params {
    int quantity = -1;
    double price = -1.0;
    std::string transaction_type;
    std::string order_type;
    std::string product;
};

/// Represents condition for a GTT.
struct gtt_condition {
    double last_price = -1.0;
    std::string exchange;
    std::string tradingsymbol;
    std::vector<double> trigger_values;
};

/// Parameters required for placing a GTT.
struct place_gtt_params {
    double last_price = -1.0;
    std::string trigger_type;
    std::string symbol;
    std::string exchange;
    std::vector<double> trigger_values;
    std::vector<gtt_order_params> orders;
};

/// Parameters required for modifying a GTT.
struct modify_gtt_params {
    int trigger_id = -1;
    double last_price = -1.0;
    std::string trigger_type;
    std::string symbol;
    std::string exchange;
    std::vector<double> trigger_values;
    std::vector<gtt_order_params> orders;
};

/// GTT represents a single Good Till Triggered order.
struct gtt {
    int id = -1;
    std::string user_id;
    std::string type;
    std::string created_at;
    std::string updated_at;
    std::string expires_at;
    std::string status;
    gtt_condition condition;
    std::vector<order> orders;
};

// JSON serialization helpers
void from_json(const json& j, gtt_order_params& p);
void from_json(const json& j, gtt_condition& c);
void from_json(const json& j, place_gtt_params& p);
void from_json(const json& j, modify_gtt_params& p);
void from_json(const json& j, gtt& g);

// Parser functions for client API
[[nodiscard]] gtt_trigger parse_gtt_trigger(json const& j);
[[nodiscard]] std::vector<gtt_trigger> parse_gtt_triggers_list(json const& j);

} // namespace cppkiteconnect
