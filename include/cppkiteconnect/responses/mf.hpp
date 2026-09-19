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

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace cppkiteconnect {

using json = nlohmann::json;

/// Parameters required for placing a mutual fund order.
struct place_mf_order_params {
    std::optional<int> quantity;
    std::optional<double> amount;
    std::string symbol;
    std::string transaction_type;
    std::optional<std::string> tag;
};

/// Parameters required for placing a mutual fund SIP.
struct place_mf_sip_params {
    int installments = 0;
    std::optional<int> installment_day;
    double amount = 0.0;
    std::optional<double> initial_amount;
    std::string symbol;
    std::string frequency;
    std::optional<std::string> tag;
};

/// Response from placing a mutual fund SIP.
struct place_mf_sip_response {
    std::string order_id;
    std::string sip_id;
};

/// Parameters required for modifying a mutual fund SIP.
struct modify_mf_sip_params {
    std::optional<int> installments;
    std::optional<int> installment_day;
    std::optional<double> amount;
    std::string sip_id;
    std::optional<std::string> status;
    std::optional<std::string> frequency;
};

/// Represents information of a mutual fund order.
struct mf_order {
    double quantity = -1.0;
    double amount = -1.0;
    double last_price = -1.0;
    double average_price = -1.0;
    
    std::string order_id;
    std::string exchange_order_id;
    std::string tradingsymbol;
    std::string status;
    std::string status_message;
    std::string folio;
    std::string fund;
    std::string order_timestamp;
    std::string exchange_timestamp;
    std::string settlement_id;
    std::string transaction_type;
    std::string variety;
    std::string purchase_type;
    std::string placed_by;
    std::string tag;
};

/// Represents information of a mutual fund holding.
struct mf_holding {
    double average_price = -1.0;
    double last_price = -1.0;
    double pnl = -1.0;
    double quantity = -1.0;
    
    std::string folio;
    std::string fund;
    std::string tradingsymbol;
    std::string last_price_date;
};

/// Represents information of a mutual fund SIP.
struct mf_sip {
    int installments = -1;
    int installment_day = -1;
    double amount = -1.0;
    double initial_amount = -1.0;
    
    std::string id;
    std::string tradingsymbol;
    std::string fund;
    std::string dividend_type;
    std::string transaction_type;
    std::string status;
    std::string sip_type;
    std::string created;
    std::string modified;
    std::string next_installment;
    std::string last_installment;
};

// JSON serialization helpers
void from_json(const json& j, place_mf_order_params& p);
void from_json(const json& j, place_mf_sip_params& p);
void from_json(const json& j, place_mf_sip_response& r);
void from_json(const json& j, modify_mf_sip_params& p);
void from_json(const json& j, mf_order& o);
void from_json(const json& j, mf_holding& h);
void from_json(const json& j, mf_sip& s);

// Parser functions for client API
[[nodiscard]] std::vector<mf_order> parse_mf_orders_list(json const& j);
[[nodiscard]] std::vector<mf_holding> parse_mf_holdings_list(json const& j);

} // namespace cppkiteconnect
