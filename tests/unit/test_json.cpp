/*
 *  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 *  SPDX-License-Identifier: MIT
 *
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

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "cppkiteconnect/responses/market.hpp"
#include "cppkiteconnect/responses/user.hpp"
#include "cppkiteconnect/responses/order.hpp"
#include "cppkiteconnect/responses/portfolio.hpp"

namespace cppkiteconnect {

using json = nlohmann::json;

TEST(JsonTest, ParseOhlc) {
    json j = R"({
        "open": 100.5,
        "high": 105.0,
        "low": 99.0,
        "close": 103.5
    })"_json;
    
    ohlc o = j.get<ohlc>();
    EXPECT_DOUBLE_EQ(o.open, 100.5);
    EXPECT_DOUBLE_EQ(o.high, 105.0);
    EXPECT_DOUBLE_EQ(o.low, 99.0);
    EXPECT_DOUBLE_EQ(o.close, 103.5);
}

TEST(JsonTest, ParseQuote) {
    json j = R"({
        "instrument_token": 123456,
        "timestamp": "2024-01-15T10:30:00+05:30",
        "last_price": 1500.50,
        "last_quantity": 100,
        "last_trade_time": "2024-01-15T10:29:55+05:30",
        "average_price": 1498.75,
        "volume": 50000,
        "buy_quantity": 25000,
        "sell_quantity": 20000,
        "net_change": 1.5,
        "oi": 100000,
        "oi_day_high": 105000,
        "oi_day_low": 95000,
        "lower_circuit_limit": 1350.0,
        "upper_circuit_limit": 1650.0,
        "ohlc": {
            "open": 1480.0,
            "high": 1520.0,
            "low": 1470.0,
            "close": 1490.0
        }
    })"_json;
    
    quote q = j.get<quote>();
    EXPECT_EQ(q.instrument_token, 123456);
    EXPECT_EQ(q.timestamp, "2024-01-15T10:30:00+05:30");
    EXPECT_DOUBLE_EQ(q.last_price, 1500.50);
    EXPECT_EQ(q.last_quantity, 100);
    EXPECT_TRUE(q.ohlc.has_value());
    EXPECT_DOUBLE_EQ(q.ohlc->open, 1480.0);
}

TEST(JsonTest, ParseUserProfile) {
    json j = R"({
        "user_id": "AB1234",
        "user_name": "Test User",
        "user_shortname": "test",
        "avatar_url": "https://example.com/avatar.png",
        "user_type": "individual",
        "email": "test@example.com",
        "broker": "zerodha",
        "products": ["BO", "CNC", "MIS"],
        "order_types": ["LIMIT", "MARKET", "SL"],
        "exchanges": ["NSE", "BSE"],
        "meta": {
            "demat_consent": "true"
        }
    })"_json;
    
    user_profile p = j.get<user_profile>();
    EXPECT_EQ(p.user_id, "AB1234");
    EXPECT_EQ(p.user_name, "Test User");
    EXPECT_EQ(p.email, "test@example.com");
    EXPECT_EQ(p.products.size(), 3);
    EXPECT_EQ(p.meta_info.demat_consent, "true");
}

TEST(JsonTest, ParseOrder) {
    json j = R"({
        "instrument_token": 123456,
        "quantity": 10,
        "disclosed_quantity": 0,
        "filled_quantity": 5,
        "pending_quantity": 5,
        "cancelled_quantity": 0,
        "validity_ttl": 0,
        "price": 1500.0,
        "trigger_price": 0.0,
        "average_price": 1500.0,
        "account_id": "AB1234",
        "placed_by": "AB1234",
        "order_id": "123456789012345",
        "exchange_order_id": "12345678",
        "parent_order_id": null,
        "status": "COMPLETE",
        "status_message": "",
        "order_timestamp": "2024-01-15T10:30:00+05:30",
        "exchange_update_timestamp": "2024-01-15T10:30:05+05:30",
        "exchange_timestamp": "2024-01-15T10:30:05+05:30",
        "rejected_by": "",
        "variety": "regular",
        "exchange": "NSE",
        "tradingsymbol": "RELIANCE",
        "order_type": "LIMIT",
        "transaction_type": "BUY",
        "validity": "DAY",
        "product": "CNC"
    })"_json;
    
    order o = j.get<order>();
    EXPECT_EQ(o.order_id, "123456789012345");
    EXPECT_EQ(o.tradingsymbol, "RELIANCE");
    EXPECT_EQ(o.status, "COMPLETE");
    EXPECT_EQ(o.quantity, 10);
    EXPECT_EQ(o.filled_quantity, 5);
    EXPECT_DOUBLE_EQ(o.price, 1500.0);
}

TEST(JsonTest, ParseHolding) {
    json j = R"({
        "instrument_token": 123456,
        "quantity": 100,
        "t1_quantity": 50,
        "realised_quantity": 0,
        "collateral_quantity": 0,
        "price": 1500.0,
        "average_price": 1450.0,
        "last_price": 1520.0,
        "close_price": 1500.0,
        "pnl": 7000.0,
        "day_change": 20.0,
        "day_change_percentage": 1.33,
        "tradingsymbol": "RELIANCE",
        "exchange": "NSE",
        "isin": "INE002A01018",
        "product": "CNC",
        "collateral_type": ""
    })"_json;
    
    holding h = j.get<holding>();
    EXPECT_EQ(h.tradingsymbol, "RELIANCE");
    EXPECT_EQ(h.quantity, 100);
    EXPECT_DOUBLE_EQ(h.average_price, 1450.0);
    EXPECT_DOUBLE_EQ(h.pnl, 7000.0);
}

TEST(JsonTest, ParsePosition) {
    json j = R"({
        "instrument_token": 123456,
        "quantity": 50,
        "buy_quantity": 50,
        "overnight_quantity": 0,
        "sell_quantity": 0,
        "day_buy_quantity": 50,
        "day_sell_quantity": 0,
        "multiplier": 1.0,
        "average_price": 1500.0,
        "close_price": 1480.0,
        "last_price": 1520.0,
        "value": 75000.0,
        "pnl": 2000.0,
        "m2m": 2000.0,
        "unrealised": 2000.0,
        "realised": 0.0,
        "buy_price": 1500.0,
        "buy_value": 75000.0,
        "buy_m2m": 2000.0,
        "sell_price": 0.0,
        "sell_value": 0.0,
        "sell_m2m": 0.0,
        "day_buy_price": 1500.0,
        "day_buy_value": 75000.0,
        "day_sell_price": 0.0,
        "day_sell_value": 0.0,
        "tradingsymbol": "RELIANCE",
        "exchange": "NSE",
        "product": "MIS"
    })"_json;
    
    position p = j.get<position>();
    EXPECT_EQ(p.tradingsymbol, "RELIANCE");
    EXPECT_EQ(p.quantity, 50);
    EXPECT_DOUBLE_EQ(p.pnl, 2000.0);
}

TEST(JsonTest, ParseInstrument) {
    json j = R"({
        "instrument_token": 123456,
        "exchange": "NSE",
        "tradingsymbol": "RELIANCE-EQ",
        "name": "RELIANCE INDUSTRIES LIMITED",
        "lot_size": 1,
        "tick_size": 0.05,
        "strike": 0.0,
        "instrument_type": "EQ",
        "expiry": "",
        "segment": "NSE"
    })"_json;
    
    instrument i = j.get<instrument>();
    EXPECT_EQ(i.tradingsymbol, "RELIANCE-EQ");
    EXPECT_EQ(i.exchange, "NSE");
    EXPECT_EQ(i.lot_size, 1);
}

TEST(JsonTest, OptionalFields) {
    // Test quote without optional depth field
    json j = R"({
        "instrument_token": 123456,
        "timestamp": "2024-01-15T10:30:00+05:30",
        "last_price": 1500.50,
        "last_quantity": 100,
        "last_trade_time": "2024-01-15T10:29:55+05:30",
        "average_price": 1498.75,
        "volume": 50000,
        "buy_quantity": 25000,
        "sell_quantity": 20000,
        "net_change": 1.5,
        "oi": 100000,
        "oi_day_high": 105000,
        "oi_day_low": 95000,
        "lower_circuit_limit": 1350.0,
        "upper_circuit_limit": 1650.0
    })"_json;
    
    quote q = j.get<quote>();
    EXPECT_FALSE(q.depth.has_value());
}

} // namespace cppkiteconnect
