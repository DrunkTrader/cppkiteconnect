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

#include <gtest/gtest.h>
#include <stdexcept>

#include "cppkiteconnect/types.hpp"
#include "cppkiteconnect/errors.hpp"

namespace cppkiteconnect {

TEST(TypesTest, ExchangeToString) {
    EXPECT_EQ(to_string(exchange::nse), "NSE");
    EXPECT_EQ(to_string(exchange::bse), "BSE");
    EXPECT_EQ(to_string(exchange::nfo), "NFO");
    EXPECT_EQ(to_string(exchange::cde), "CDE");
    EXPECT_EQ(to_string(exchange::bfo), "BFO");
    EXPECT_EQ(to_string(exchange::mcx), "MCX");
}

TEST(TypesTest, ExchangeFromString) {
    EXPECT_EQ(exchange_from_string("NSE"), exchange::nse);
    EXPECT_EQ(exchange_from_string("BSE"), exchange::bse);
    EXPECT_EQ(exchange_from_string("NFO"), exchange::nfo);
    EXPECT_EQ(exchange_from_string("CDE"), exchange::cde);
    EXPECT_EQ(exchange_from_string("BFO"), exchange::bfo);
    EXPECT_EQ(exchange_from_string("MCX"), exchange::mcx);
    
    EXPECT_THROW(exchange_from_string("INVALID"), std::invalid_argument);
}

TEST(TypesTest, TransactionTypeToString) {
    EXPECT_EQ(to_string(transaction_type::buy), "BUY");
    EXPECT_EQ(to_string(transaction_type::sell), "SELL");
}

TEST(TypesTest, TransactionTypeFromString) {
    EXPECT_EQ(transaction_type_from_string("BUY"), transaction_type::buy);
    EXPECT_EQ(transaction_type_from_string("SELL"), transaction_type::sell);
    EXPECT_THROW(transaction_type_from_string("INVALID"), std::invalid_argument);
}

TEST(TypesTest, ProductToString) {
    EXPECT_EQ(to_string(product::CNC), "CNC");
    EXPECT_EQ(to_string(product::NRML), "NRML");
    EXPECT_EQ(to_string(product::MIS), "MIS");
    EXPECT_EQ(to_string(product::CO), "CO");
    EXPECT_EQ(to_string(product::BO), "BO");
}

TEST(TypesTest, ProductFromString) {
    EXPECT_EQ(product_from_string("CNC"), product::CNC);
    EXPECT_EQ(product_from_string("NRML"), product::NRML);
    EXPECT_EQ(product_from_string("MIS"), product::MIS);
    EXPECT_EQ(product_from_string("CO"), product::CO);
    EXPECT_EQ(product_from_string("BO"), product::BO);
    EXPECT_THROW(product_from_string("INVALID"), std::invalid_argument);
}

TEST(TypesTest, OrderTypeToString) {
    EXPECT_EQ(to_string(order_type::MARKET), "MARKET");
    EXPECT_EQ(to_string(order_type::LIMIT), "LIMIT");
    EXPECT_EQ(to_string(order_type::SL), "SL");
    EXPECT_EQ(to_string(order_type::SLM), "SLM");
}

TEST(TypesTest, OrderTypeFromString) {
    EXPECT_EQ(order_type_from_string("MARKET"), order_type::MARKET);
    EXPECT_EQ(order_type_from_string("LIMIT"), order_type::LIMIT);
    EXPECT_EQ(order_type_from_string("SL"), order_type::SL);
    EXPECT_EQ(order_type_from_string("SLM"), order_type::SLM);
    EXPECT_THROW(order_type_from_string("INVALID"), std::invalid_argument);
}

TEST(TypesTest, ValidityToString) {
    EXPECT_EQ(to_string(validity::DAY), "DAY");
    EXPECT_EQ(to_string(validity::IOC), "IOC");
    EXPECT_EQ(to_string(validity::TTL), "TTL");
}

TEST(TypesTest, ValidityFromString) {
    EXPECT_EQ(validity_from_string("DAY"), validity::DAY);
    EXPECT_EQ(validity_from_string("IOC"), validity::IOC);
    EXPECT_EQ(validity_from_string("TTL"), validity::TTL);
    EXPECT_THROW(validity_from_string("INVALID"), std::invalid_argument);
}

TEST(TypesTest, VarietyToString) {
    EXPECT_EQ(to_string(variety::REGULAR), "regular");
    EXPECT_EQ(to_string(variety::BO), "bo");
    EXPECT_EQ(to_string(variety::CO), "co");
    EXPECT_EQ(to_string(variety::AMO), "amo");
}

TEST(TypesTest, VarietyFromString) {
    EXPECT_EQ(variety_from_string("regular"), variety::REGULAR);
    EXPECT_EQ(variety_from_string("bo"), variety::BO);
    EXPECT_EQ(variety_from_string("co"), variety::CO);
    EXPECT_EQ(variety_from_string("amo"), variety::AMO);
    EXPECT_THROW(variety_from_string("INVALID"), std::invalid_argument);
}

TEST(TypesTest, PositionTypeToString) {
    EXPECT_EQ(to_string(position_type::net), "net");
    EXPECT_EQ(to_string(position_type::day), "day");
}

TEST(TypesTest, PositionTypeFromString) {
    EXPECT_EQ(position_type_from_string("net"), position_type::net);
    EXPECT_EQ(position_type_from_string("day"), position_type::day);
    EXPECT_THROW(position_type_from_string("INVALID"), std::invalid_argument);
}

TEST(TypesTest, SubscriptionModeToString) {
    EXPECT_EQ(to_string(subscription_mode::quote), "quote");
    EXPECT_EQ(to_string(subscription_mode::ltp), "ltp");
    EXPECT_EQ(to_string(subscription_mode::full), "full");
}

TEST(TypesTest, SubscriptionModeFromString) {
    EXPECT_EQ(subscription_mode_from_string("quote"), subscription_mode::quote);
    EXPECT_EQ(subscription_mode_from_string("ltp"), subscription_mode::ltp);
    EXPECT_EQ(subscription_mode_from_string("full"), subscription_mode::full);
    EXPECT_THROW(subscription_mode_from_string("INVALID"), std::invalid_argument);
}

TEST(TypesTest, ConnectionStateToString) {
    EXPECT_EQ(to_string(connection_state::disconnected), "disconnected");
    EXPECT_EQ(to_string(connection_state::connecting), "connecting");
    EXPECT_EQ(to_string(connection_state::connected), "connected");
    EXPECT_EQ(to_string(connection_state::reconnecting), "reconnecting");
    EXPECT_EQ(to_string(connection_state::stopping), "stopping");
}

TEST(TypesTest, ConnectionStateFromString) {
    EXPECT_EQ(connection_state_from_string("disconnected"), connection_state::disconnected);
    EXPECT_EQ(connection_state_from_string("connecting"), connection_state::connecting);
    EXPECT_EQ(connection_state_from_string("connected"), connection_state::connected);
    EXPECT_EQ(connection_state_from_string("reconnecting"), connection_state::reconnecting);
    EXPECT_EQ(connection_state_from_string("stopping"), connection_state::stopping);
    EXPECT_THROW(connection_state_from_string("INVALID"), std::invalid_argument);
}

TEST(ErrorsTest, KiteExceptionBasics) {
    error_info info{
        error_code::authentication_failed,
        401,
        "Authentication failed",
        "TokenExpired",
        "req-123"
    };
    
    kite_exception ex(info);
    
    EXPECT_STREQ(ex.what(), "Authentication failed");
    EXPECT_EQ(ex.code(), error_code::authentication_failed);
    EXPECT_EQ(ex.http_status(), 401);
    EXPECT_EQ(ex.api_error_type(), "TokenExpired");
    EXPECT_EQ(ex.request_id(), "req-123");
}

TEST(ErrorsTest, RateLimitErrorRetryAfter) {
    error_info info{error_code::rate_limited, 429, "Rate limit exceeded", "", ""};
    
    rate_limit_error ex(info, std::chrono::seconds(60));
    
    EXPECT_EQ(ex.retry_after(), std::chrono::seconds(60));
}

TEST(ErrorsTest, RetryPolicyBasics) {
    retry_policy policy;
    
    EXPECT_TRUE(policy.is_retryable(error_code::network_failure));
    EXPECT_TRUE(policy.is_retryable(error_code::timeout));
    EXPECT_FALSE(policy.is_retryable(error_code::authentication_failed));
    EXPECT_FALSE(policy.is_retryable(error_code::authorization_failed));
    EXPECT_FALSE(policy.is_retryable(error_code::invalid_request));
    EXPECT_FALSE(policy.is_retryable(error_code::order_rejected));
}

TEST(ErrorsTest, RetryPolicyDelayCalculation) {
    retry_policy policy{
        5,                              // max_attempts
        std::chrono::milliseconds(100), // initial_delay
        std::chrono::milliseconds(5000),// max_delay
        2.0                             // backoff_multiplier
    };
    
    EXPECT_EQ(policy.delay_for_attempt(1), std::chrono::milliseconds(100));
    EXPECT_EQ(policy.delay_for_attempt(2), std::chrono::milliseconds(200));
    EXPECT_EQ(policy.delay_for_attempt(3), std::chrono::milliseconds(400));
    EXPECT_EQ(policy.delay_for_attempt(4), std::chrono::milliseconds(800));
    EXPECT_EQ(policy.delay_for_attempt(5), std::chrono::milliseconds(1600));
}

TEST(RequestsTest, OrderRequestDefaults) {
    order_request req;
    
    EXPECT_EQ(req.exch, exchange::nse);
    EXPECT_EQ(req.txn_type, transaction_type::buy);
    EXPECT_EQ(req.prod, product::CNC);
    EXPECT_EQ(req.ord_type, order_type::MARKET);
    EXPECT_EQ(req.valid, validity::DAY);
    EXPECT_EQ(req.var, variety::REGULAR);
    EXPECT_EQ(req.quantity, 0);
    EXPECT_FALSE(req.price.has_value());
    EXPECT_FALSE(req.trigger_price.has_value());
}

TEST(RequestsTest, OrderRequestFluentInterface) {
    order_request req;
    req.set_exchange(exchange::nfo)
       .set_tradingsymbol("NIFTY24DEC22000CE")
       .set_transaction_type(transaction_type::buy)
       .set_product(product::NRML)
       .set_order_type(order_type::LIMIT)
       .set_validity(validity::DAY)
       .set_quantity(100)
       .set_price(150.50);
    
    EXPECT_EQ(req.exch, exchange::nfo);
    EXPECT_EQ(req.tradingsymbol, "NIFTY24DEC22000CE");
    EXPECT_EQ(req.txn_type, transaction_type::buy);
    EXPECT_EQ(req.prod, product::NRML);
    EXPECT_EQ(req.ord_type, order_type::LIMIT);
    EXPECT_EQ(req.quantity, 100);
    EXPECT_EQ(req.price.value(), 150.50);
}

} // namespace cppkiteconnect
