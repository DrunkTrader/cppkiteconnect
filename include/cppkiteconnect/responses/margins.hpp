/*
 *  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 *  SPDX-License-Identifier: MIT
 *
 *  Copyright (c) 2020-2023 Bhumit Attarde
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

namespace cppkiteconnect {

using json = nlohmann::json;

/// Represents GST charges.
struct gst_charges {
    double igst = -1.0;
    double cgst = -1.0;
    double sgst = -1.0;
    double total = -1.0;
};

/// Represents order charges.
struct order_charges {
    double transaction_tax = -1.0;
    double exchange_turnover_charge = -1.0;
    double sebi_turnover_charge = -1.0;
    double brokerage = -1.0;
    double stamp_duty = -1.0;
    double total = -1.0;
    std::string transaction_tax_type;
    gst_charges gst;
};

/// Parameters required by the margin methods.
struct margins_params {
    double quantity = 0.0;
    double price = 0.0;
    double trigger_price = 0.0;
    std::string exchange;
    std::string tradingsymbol;
    std::string transaction_type;
    std::string variety;
    std::string product;
    std::string order_type;
};

/// Represents P&L for order margins.
struct pnl_info {
    double realised = -1.0;
    double unrealised = -1.0;
};

/// Represents margins required for placing an order.
struct order_margins {
    double span = -1.0;
    double exposure = -1.0;
    double option_premium = -1.0;
    double additional = -1.0;
    double bo = -1.0;
    double cash = -1.0;
    double var = -1.0;
    double total = -1.0;
    double leverage = -1.0;
    
    std::string type;
    std::string tradingsymbol;
    std::string exchange;
    pnl_info pnl;
    order_charges charges;
};

/// Represents basket margins for multiple orders.
struct basket_margins {
    order_margins initial;
    order_margins final;
    std::vector<order_margins> orders;
    order_charges charges;
};

// JSON serialization helpers
void from_json(const json& j, gst_charges& g);
void from_json(const json& j, order_charges& c);
void from_json(const json& j, margins_params& p);
void from_json(const json& j, pnl_info& p);
void from_json(const json& j, order_margins& m);
void from_json(const json& j, basket_margins& b);

} // namespace cppkiteconnect
