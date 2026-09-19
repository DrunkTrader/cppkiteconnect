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

#include "cppkiteconnect/types.hpp"
#include <stdexcept>

namespace cppkiteconnect {

std::string to_string(exchange e) {
    switch (e) {
        case exchange::nse: return "NSE";
        case exchange::bse: return "BSE";
        case exchange::nfo: return "NFO";
        case exchange::cde: return "CDE";
        case exchange::bfo: return "BFO";
        case exchange::mcx: return "MCX";
        default: throw std::invalid_argument("Unknown exchange value");
    }
}

exchange exchange_from_string(std::string_view s) {
    if (s == "NSE") return exchange::nse;
    if (s == "BSE") return exchange::bse;
    if (s == "NFO") return exchange::nfo;
    if (s == "CDE") return exchange::cde;
    if (s == "BFO") return exchange::bfo;
    if (s == "MCX") return exchange::mcx;
    throw std::invalid_argument(std::string("Unknown exchange: ") + std::string(s));
}

std::string to_string(transaction_type t) {
    switch (t) {
        case transaction_type::buy: return "BUY";
        case transaction_type::sell: return "SELL";
        default: throw std::invalid_argument("Unknown transaction_type value");
    }
}

transaction_type transaction_type_from_string(std::string_view s) {
    if (s == "BUY") return transaction_type::buy;
    if (s == "SELL") return transaction_type::sell;
    throw std::invalid_argument(std::string("Unknown transaction_type: ") + std::string(s));
}

std::string to_string(product p) {
    switch (p) {
        case product::CNC: return "CNC";
        case product::NRML: return "NRML";
        case product::MIS: return "MIS";
        case product::CO: return "CO";
        case product::BO: return "BO";
        default: throw std::invalid_argument("Unknown product value");
    }
}

product product_from_string(std::string_view s) {
    if (s == "CNC") return product::CNC;
    if (s == "NRML") return product::NRML;
    if (s == "MIS") return product::MIS;
    if (s == "CO") return product::CO;
    if (s == "BO") return product::BO;
    throw std::invalid_argument(std::string("Unknown product: ") + std::string(s));
}

std::string to_string(order_type o) {
    switch (o) {
        case order_type::MARKET: return "MARKET";
        case order_type::LIMIT: return "LIMIT";
        case order_type::SL: return "SL";
        case order_type::SLM: return "SLM";
        default: throw std::invalid_argument("Unknown order_type value");
    }
}

order_type order_type_from_string(std::string_view s) {
    if (s == "MARKET") return order_type::MARKET;
    if (s == "LIMIT") return order_type::LIMIT;
    if (s == "SL") return order_type::SL;
    if (s == "SLM") return order_type::SLM;
    throw std::invalid_argument(std::string("Unknown order_type: ") + std::string(s));
}

std::string to_string(validity v) {
    switch (v) {
        case validity::DAY: return "DAY";
        case validity::IOC: return "IOC";
        case validity::TTL: return "TTL";
        default: throw std::invalid_argument("Unknown validity value");
    }
}

validity validity_from_string(std::string_view s) {
    if (s == "DAY") return validity::DAY;
    if (s == "IOC") return validity::IOC;
    if (s == "TTL") return validity::TTL;
    throw std::invalid_argument(std::string("Unknown validity: ") + std::string(s));
}

std::string to_string(variety v) {
    switch (v) {
        case variety::REGULAR: return "regular";
        case variety::BO: return "bo";
        case variety::CO: return "co";
        case variety::AMO: return "amo";
        default: throw std::invalid_argument("Unknown variety value");
    }
}

variety variety_from_string(std::string_view s) {
    if (s == "regular") return variety::REGULAR;
    if (s == "bo") return variety::BO;
    if (s == "co") return variety::CO;
    if (s == "amo") return variety::AMO;
    throw std::invalid_argument(std::string("Unknown variety: ") + std::string(s));
}

std::string to_string(position_type p) {
    switch (p) {
        case position_type::net: return "net";
        case position_type::day: return "day";
        default: throw std::invalid_argument("Unknown position_type value");
    }
}

position_type position_type_from_string(std::string_view s) {
    if (s == "net") return position_type::net;
    if (s == "day") return position_type::day;
    throw std::invalid_argument(std::string("Unknown position_type: ") + std::string(s));
}

std::string to_string(subscription_mode m) {
    switch (m) {
        case subscription_mode::quote: return "quote";
        case subscription_mode::ltp: return "ltp";
        case subscription_mode::full: return "full";
        default: throw std::invalid_argument("Unknown subscription_mode value");
    }
}

subscription_mode subscription_mode_from_string(std::string_view s) {
    if (s == "quote") return subscription_mode::quote;
    if (s == "ltp") return subscription_mode::ltp;
    if (s == "full") return subscription_mode::full;
    throw std::invalid_argument(std::string("Unknown subscription_mode: ") + std::string(s));
}

std::string to_string(connection_state s) {
    switch (s) {
        case connection_state::disconnected: return "disconnected";
        case connection_state::connecting: return "connecting";
        case connection_state::connected: return "connected";
        case connection_state::reconnecting: return "reconnecting";
        case connection_state::stopping: return "stopping";
        default: throw std::invalid_argument("Unknown connection_state value");
    }
}

connection_state connection_state_from_string(std::string_view s) {
    if (s == "disconnected") return connection_state::disconnected;
    if (s == "connecting") return connection_state::connecting;
    if (s == "connected") return connection_state::connected;
    if (s == "reconnecting") return connection_state::reconnecting;
    if (s == "stopping") return connection_state::stopping;
    throw std::invalid_argument(std::string("Unknown connection_state: ") + std::string(s));
}

// Subscription mode conversions (Phase 7)
std::string to_string(subscription_mode mode) {
    switch (mode) {
        case subscription_mode::quote: return "quote";
        case subscription_mode::ltp: return "ltp";
        case subscription_mode::full: return "full";
        default: return "unknown";
    }
}

subscription_mode subscription_mode_from_string(std::string_view s) {
    if (s == "quote") return subscription_mode::quote;
    if (s == "ltp") return subscription_mode::ltp;
    if (s == "full") return subscription_mode::full;
    throw std::invalid_argument{std::format("Invalid subscription mode: {}", s)};
}

} // namespace cppkiteconnect
