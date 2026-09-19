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

#include "cppkiteconnect/responses/market.hpp"
#include "cppkiteconnect/responses/user.hpp"
#include "cppkiteconnect/responses/order.hpp"
#include "cppkiteconnect/responses/portfolio.hpp"
#include "cppkiteconnect/responses/gtt.hpp"
#include "cppkiteconnect/responses/mf.hpp"
#include "cppkiteconnect/responses/margins.hpp"
#include "cppkiteconnect/responses/ws.hpp"
#include "cppkiteconnect/errors.hpp"
#include <fmt/core.h>

namespace cppkiteconnect {

// Helper function to extract data from Kite Connect response envelope
template<typename T>
T extract_data(nlohmann::json const& j) {
    if (!j.contains("data")) {
        throw parse_error(error_info{
            .code = "MISSING_DATA",
            .message = "Response missing 'data' field"
        });
    }
    return j.at("data").get<T>();
}

// Authentication response (special case - no envelope for session token)
user_session parse_auth_response(nlohmann::json const& j) {
    return extract_data<user_session>(j);
}

// User profile
user_profile parse_user_profile(nlohmann::json const& j) {
    return extract_data<user_profile>(j);
}

// Margins
margins parse_margins_response(nlohmann::json const& j) {
    return extract_data<margins>(j);
}

// Order response (single order)
order parse_order_response(nlohmann::json const& j) {
    return extract_data<order>(j);
}

// Orders list
std::vector<order> parse_orders_list(nlohmann::json const& j) {
    return extract_data<std::vector<order>>(j);
}

// Holdings list
std::vector<holding> parse_holdings_list(nlohmann::json const& j) {
    return extract_data<std::vector<holding>>(j);
}

// Positions list
std::vector<position> parse_positions_list(nlohmann::json const& j) {
    auto data = extract_data<positions_response>(j);
    return data.net;
}

// Quote data
quote_data parse_quote_data(nlohmann::json const& j) {
    return extract_data<quote_data>(j);
}

// OHLC data
ohlc_data parse_ohlc_data(nlohmann::json const& j) {
    return extract_data<ohlc_data>(j);
}

// Historical candles
std::vector<candle> parse_candles(nlohmann::json const& j) {
    return extract_data<std::vector<candle>>(j);
}

// GTT trigger
gtt_trigger parse_gtt_trigger(nlohmann::json const& j) {
    return extract_data<gtt_trigger>(j);
}

// GTT triggers list
std::vector<gtt_trigger> parse_gtt_triggers_list(nlohmann::json const& j) {
    return extract_data<std::vector<gtt_trigger>>(j);
}

// MF orders list
std::vector<mf_order> parse_mf_orders_list(nlohmann::json const& j) {
    return extract_data<std::vector<mf_order>>(j);
}

// MF holdings list
std::vector<mf_holding> parse_mf_holdings_list(nlohmann::json const& j) {
    return extract_data<std::vector<mf_holding>>(j);
}

// Market responses
void from_json(const json& j, ohlc& o) {
    j.at("open").get_to(o.open);
    j.at("high").get_to(o.high);
    j.at("low").get_to(o.low);
    j.at("close").get_to(o.close);
}

void from_json(const json& j, market_depth_entry& e) {
    j.at("price").get_to(e.price);
    j.at("quantity").get_to(e.quantity);
    j.at("orders").get_to(e.orders);
}

void from_json(const json& j, market_depth& d) {
    if (j.contains("buy")) {
        j.at("buy").get_to(d.buy);
    }
    if (j.contains("sell")) {
        j.at("sell").get_to(d.sell);
    }
}

void from_json(const json& j, quote& q) {
    j.at("instrument_token").get_to(q.instrument_token);
    j.at("timestamp").get_to(q.timestamp);
    j.at("last_price").get_to(q.last_price);
    j.at("last_quantity").get_to(q.last_quantity);
    j.at("last_trade_time").get_to(q.last_trade_time);
    j.at("average_price").get_to(q.average_price);
    j.at("volume").get_to(q.volume);
    j.at("buy_quantity").get_to(q.buy_quantity);
    j.at("sell_quantity").get_to(q.sell_quantity);
    
    if (j.contains("ohlc")) {
        q.ohlc = j.at("ohlc").get<ohlc>();
    }
    
    j.at("net_change").get_to(q.net_change);
    j.at("oi").get_to(q.oi);
    j.at("oi_day_high").get_to(q.oi_day_high);
    j.at("oi_day_low").get_to(q.oi_day_low);
    j.at("lower_circuit_limit").get_to(q.lower_circuit_limit);
    j.at("upper_circuit_limit").get_to(q.upper_circuit_limit);
    
    if (j.contains("depth")) {
        q.depth = j.at("depth").get<market_depth>();
    }
}

void from_json(const json& j, ohlc_quote& oq) {
    j.at("instrument_token").get_to(oq.instrument_token);
    if (j.contains("ohlc")) {
        oq.ohlc = j.at("ohlc").get<ohlc>();
    }
}

void from_json(const json& j, instrument& i) {
    j.at("instrument_token").get_to(i.instrument_token);
    j.at("exchange").get_to(i.exchange);
    j.at("tradingsymbol").get_to(i.tradingsymbol);
    j.at("name").get_to(i.name);
    j.at("lot_size").get_to(i.lot_size);
    j.at("tick_size").get_to(i.tick_size);
    j.at("strike").get_to(i.strike);
    j.at("instrument_type").get_to(i.instrument_type);
    j.at("expiry").get_to(i.expiry);
    j.at("segment").get_to(i.segment);
}

// User responses
void from_json(const json& j, user_profile::meta& m) {
    j.at("demat_consent").get_to(m.demat_consent);
}

void from_json(const json& j, user_profile& p) {
    j.at("user_id").get_to(p.user_id);
    j.at("user_name").get_to(p.user_name);
    j.at("user_shortname").get_to(p.user_shortname);
    j.at("avatar_url").get_to(p.avatar_url);
    j.at("user_type").get_to(p.user_type);
    j.at("email").get_to(p.email);
    j.at("broker").get_to(p.broker);
    j.at("products").get_to(p.products);
    j.at("order_types").get_to(p.order_types);
    j.at("exchanges").get_to(p.exchanges);
    if (j.contains("meta")) {
        p.meta_info = j.at("meta").get<user_profile::meta>();
    }
}

void from_json(const json& j, user_tokens& t) {
    j.at("user_id").get_to(t.user_id);
    j.at("access_token").get_to(t.access_token);
    j.at("refresh_token").get_to(t.refresh_token);
}

void from_json(const json& j, user_session& s) {
    s.profile = j.get<user_profile>();
    s.tokens = j.get<user_tokens>();
    j.at("api_key").get_to(s.api_key);
    j.at("public_token").get_to(s.public_token);
    j.at("login_time").get_to(s.login_time);
}

void from_json(const json& j, available_margins& m) {
    j.at("adhoc_margin").get_to(m.adhoc_margin);
    j.at("cash").get_to(m.cash);
    j.at("collateral").get_to(m.collateral);
    j.at("intraday_payin").get_to(m.intraday_payin);
}

void from_json(const json& j, used_margins& m) {
    j.at("debits").get_to(m.debits);
    j.at("exposure").get_to(m.exposure);
    j.at("m2m_realised").get_to(m.m2m_realised);
    j.at("m2m_unrealised").get_to(m.m2m_unrealised);
    j.at("option_premium").get_to(m.option_premium);
    j.at("payout").get_to(m.payout);
    j.at("span").get_to(m.span);
    j.at("holding_sales").get_to(m.holding_sales);
    j.at("turnover").get_to(m.turnover);
}

void from_json(const json& j, margins& m) {
    j.at("net").get_to(m.net);
    j.at("enabled").get_to(m.enabled);
    j.at("available").get_to(m.available);
    j.at("utilised").get_to(m.utilised);
}

void from_json(const json& j, all_margins& m) {
    j.at("equity").get_to(m.equity);
    j.at("commodity").get_to(m.commodity);
}

// Order responses
void from_json(const json& j, order& o) {
    j.at("instrument_token").get_to(o.instrument_token);
    j.at("quantity").get_to(o.quantity);
    j.at("disclosed_quantity").get_to(o.disclosed_quantity);
    j.at("filled_quantity").get_to(o.filled_quantity);
    j.at("pending_quantity").get_to(o.pending_quantity);
    j.at("cancelled_quantity").get_to(o.cancelled_quantity);
    j.at("validity_ttl").get_to(o.validity_ttl);
    j.at("price").get_to(o.price);
    j.at("trigger_price").get_to(o.trigger_price);
    j.at("average_price").get_to(o.average_price);
    j.at("account_id").get_to(o.account_id);
    j.at("placed_by").get_to(o.placed_by);
    j.at("order_id").get_to(o.order_id);
    j.at("exchange_order_id").get_to(o.exchange_order_id);
    j.at("parent_order_id").get_to(o.parent_order_id);
    j.at("status").get_to(o.status);
    j.at("status_message").get_to(o.status_message);
    j.at("order_timestamp").get_to(o.order_timestamp);
    j.at("exchange_update_timestamp").get_to(o.exchange_update_timestamp);
    j.at("exchange_timestamp").get_to(o.exchange_timestamp);
    j.at("rejected_by").get_to(o.rejected_by);
    j.at("variety").get_to(o.variety);
    j.at("exchange").get_to(o.exchange);
    j.at("tradingsymbol").get_to(o.tradingsymbol);
    j.at("order_type").get_to(o.order_type);
    j.at("transaction_type").get_to(o.transaction_type);
    j.at("validity").get_to(o.validity);
    j.at("product").get_to(o.product);
}

void from_json(const json& j, trade& t) {
    j.at("instrument_token").get_to(t.instrument_token);
    j.at("average_price").get_to(t.average_price);
    j.at("quantity").get_to(t.quantity);
    j.at("trade_id").get_to(t.trade_id);
    j.at("product").get_to(t.product);
    j.at("fill_timestamp").get_to(t.fill_timestamp);
    j.at("exchange_timestamp").get_to(t.exchange_timestamp);
    j.at("exchange_order_id").get_to(t.exchange_order_id);
    j.at("order_id").get_to(t.order_id);
    j.at("transaction_type").get_to(t.transaction_type);
    j.at("tradingsymbol").get_to(t.tradingsymbol);
    j.at("exchange").get_to(t.exchange);
}

// Portfolio responses
void from_json(const json& j, holding& h) {
    j.at("instrument_token").get_to(h.instrument_token);
    j.at("quantity").get_to(h.quantity);
    j.at("t1_quantity").get_to(h.t1_quantity);
    j.at("realised_quantity").get_to(h.realised_quantity);
    j.at("collateral_quantity").get_to(h.collateral_quantity);
    j.at("price").get_to(h.price);
    j.at("average_price").get_to(h.average_price);
    j.at("last_price").get_to(h.last_price);
    j.at("close_price").get_to(h.close_price);
    j.at("pnl").get_to(h.pnl);
    j.at("day_change").get_to(h.day_change);
    j.at("day_change_percentage").get_to(h.day_change_percentage);
    j.at("tradingsymbol").get_to(h.tradingsymbol);
    j.at("exchange").get_to(h.exchange);
    j.at("isin").get_to(h.isin);
    j.at("product").get_to(h.product);
    j.at("collateral_type").get_to(h.collateral_type);
}

void from_json(const json& j, position& p) {
    j.at("instrument_token").get_to(p.instrument_token);
    j.at("quantity").get_to(p.quantity);
    j.at("buy_quantity").get_to(p.buy_quantity);
    j.at("overnight_quantity").get_to(p.overnight_quantity);
    j.at("sell_quantity").get_to(p.sell_quantity);
    j.at("day_buy_quantity").get_to(p.day_buy_quantity);
    j.at("day_sell_quantity").get_to(p.day_sell_quantity);
    j.at("multiplier").get_to(p.multiplier);
    j.at("average_price").get_to(p.average_price);
    j.at("close_price").get_to(p.close_price);
    j.at("last_price").get_to(p.last_price);
    j.at("value").get_to(p.value);
    j.at("pnl").get_to(p.pnl);
    j.at("m2m").get_to(p.m2m);
    j.at("unrealised").get_to(p.unrealised);
    j.at("realised").get_to(p.realised);
    j.at("buy_price").get_to(p.buy_price);
    j.at("buy_value").get_to(p.buy_value);
    j.at("buy_m2m").get_to(p.buy_m2m);
    j.at("sell_price").get_to(p.sell_price);
    j.at("sell_value").get_to(p.sell_value);
    j.at("sell_m2m").get_to(p.sell_m2m);
    j.at("day_buy_price").get_to(p.day_buy_price);
    j.at("day_buy_value").get_to(p.day_buy_value);
    j.at("day_sell_price").get_to(p.day_sell_price);
    j.at("day_sell_value").get_to(p.day_sell_value);
    j.at("tradingsymbol").get_to(p.tradingsymbol);
    j.at("exchange").get_to(p.exchange);
    j.at("product").get_to(p.product);
}

void from_json(const json& j, positions_response& pr) {
    if (j.contains("net")) {
        j.at("net").get_to(pr.net);
    }
    if (j.contains("day")) {
        j.at("day").get_to(pr.day);
    }
}

// GTT responses
void from_json(const json& j, gtt_order_params& p) {
    j.at("quantity").get_to(p.quantity);
    j.at("price").get_to(p.price);
    j.at("transaction_type").get_to(p.transaction_type);
    j.at("order_type").get_to(p.order_type);
    j.at("product").get_to(p.product);
}

void from_json(const json& j, gtt_condition& c) {
    j.at("exchange").get_to(c.exchange);
    j.at("tradingsymbol").get_to(c.tradingsymbol);
    j.at("last_price").get_to(c.last_price);
    j.at("trigger_values").get_to(c.trigger_values);
}

void from_json(const json& j, gtt& g) {
    j.at("id").get_to(g.id);
    j.at("user_id").get_to(g.user_id);
    j.at("type").get_to(g.type);
    j.at("created_at").get_to(g.created_at);
    j.at("updated_at").get_to(g.updated_at);
    j.at("expires_at").get_to(g.expires_at);
    j.at("status").get_to(g.status);
    j.at("condition").get_to(g.condition);
    j.at("orders").get_to(g.orders);
}

// MF responses
void from_json(const json& j, mf_order& o) {
    j.at("quantity").get_to(o.quantity);
    j.at("amount").get_to(o.amount);
    j.at("last_price").get_to(o.last_price);
    j.at("average_price").get_to(o.average_price);
    j.at("order_id").get_to(o.order_id);
    j.at("exchange_order_id").get_to(o.exchange_order_id);
    j.at("tradingsymbol").get_to(o.tradingsymbol);
    j.at("status").get_to(o.status);
    j.at("status_message").get_to(o.status_message);
    j.at("folio").get_to(o.folio);
    j.at("fund").get_to(o.fund);
    j.at("order_timestamp").get_to(o.order_timestamp);
    j.at("exchange_timestamp").get_to(o.exchange_timestamp);
    j.at("settlement_id").get_to(o.settlement_id);
    j.at("transaction_type").get_to(o.transaction_type);
    j.at("variety").get_to(o.variety);
    j.at("purchase_type").get_to(o.purchase_type);
    j.at("placed_by").get_to(o.placed_by);
    j.at("tag").get_to(o.tag);
}

void from_json(const json& j, mf_holding& h) {
    j.at("average_price").get_to(h.average_price);
    j.at("last_price").get_to(h.last_price);
    j.at("pnl").get_to(h.pnl);
    j.at("quantity").get_to(h.quantity);
    j.at("folio").get_to(h.folio);
    j.at("fund").get_to(h.fund);
    j.at("tradingsymbol").get_to(h.tradingsymbol);
    j.at("last_price_date").get_to(h.last_price_date);
}

void from_json(const json& j, mf_sip& s) {
    j.at("installments").get_to(s.installments);
    j.at("installment_day").get_to(s.installment_day);
    j.at("amount").get_to(s.amount);
    j.at("initial_amount").get_to(s.initial_amount);
    j.at("sip_id").get_to(s.id);
    j.at("tradingsymbol").get_to(s.tradingsymbol);
    j.at("fund").get_to(s.fund);
    j.at("dividend_type").get_to(s.dividend_type);
    j.at("transaction_type").get_to(s.transaction_type);
    j.at("status").get_to(s.status);
    j.at("sip_type").get_to(s.sip_type);
    j.at("created").get_to(s.created);
    j.at("modified").get_to(s.modified);
    j.at("next_installment").get_to(s.next_installment);
    j.at("last_installment").get_to(s.last_installment);
}

// Margins responses
void from_json(const json& j, gst_charges& g) {
    j.at("igst").get_to(g.igst);
    j.at("cgst").get_to(g.cgst);
    j.at("sgst").get_to(g.sgst);
    j.at("total").get_to(g.total);
}

void from_json(const json& j, order_charges& c) {
    j.at("transaction_tax").get_to(c.transaction_tax);
    j.at("exchange_turnover_charge").get_to(c.exchange_turnover_charge);
    j.at("sebi_turnover_charge").get_to(c.sebi_turnover_charge);
    j.at("brokerage").get_to(c.brokerage);
    j.at("stamp_duty").get_to(c.stamp_duty);
    j.at("total").get_to(c.total);
    j.at("transaction_tax_type").get_to(c.transaction_tax_type);
    j.at("gst").get_to(c.gst);
}

void from_json(const json& j, pnl_info& p) {
    j.at("realised").get_to(p.realised);
    j.at("unrealised").get_to(p.unrealised);
}

void from_json(const json& j, order_margins& m) {
    j.at("span").get_to(m.span);
    j.at("exposure").get_to(m.exposure);
    j.at("option_premium").get_to(m.option_premium);
    j.at("additional").get_to(m.additional);
    j.at("bo").get_to(m.bo);
    j.at("cash").get_to(m.cash);
    j.at("var").get_to(m.var);
    j.at("total").get_to(m.total);
    j.at("leverage").get_to(m.leverage);
    j.at("type").get_to(m.type);
    j.at("tradingsymbol").get_to(m.tradingsymbol);
    j.at("exchange").get_to(m.exchange);
    j.at("pnl").get_to(m.pnl);
    j.at("charges").get_to(m.charges);
}

void from_json(const json& j, basket_margins& b) {
    j.at("initial").get_to(b.initial);
    j.at("final").get_to(b.final);
    j.at("orders").get_to(b.orders);
    j.at("charges").get_to(b.charges);
}

// WebSocket responses
void from_json(const json& j, depth_ws& d) {
    j.at("orders").get_to(d.orders);
    j.at("quantity").get_to(d.quantity);
    j.at("price").get_to(d.price);
}

void from_json(const json& j, tick_ohlc& o) {
    j.at("open").get_to(o.open);
    j.at("high").get_to(o.high);
    j.at("low").get_to(o.low);
    j.at("close").get_to(o.close);
}

void from_json(const json& j, tick_market_depth& m) {
    if (j.contains("buy")) {
        j.at("buy").get_to(m.buy);
    }
    if (j.contains("sell")) {
        j.at("sell").get_to(m.sell);
    }
}

void from_json(const json& j, tick& t) {
    j.at("instrument_token").get_to(t.instrument_token);
    j.at("timestamp").get_to(t.timestamp);
    j.at("last_trade_time").get_to(t.last_trade_time);
    j.at("last_traded_quantity").get_to(t.last_traded_quantity);
    j.at("total_buy_quantity").get_to(t.total_buy_quantity);
    j.at("total_sell_quantity").get_to(t.total_sell_quantity);
    j.at("volume_traded").get_to(t.volume_traded);
    j.at("oi").get_to(t.oi);
    j.at("oi_day_high").get_to(t.oi_day_high);
    j.at("oi_day_low").get_to(t.oi_day_low);
    j.at("mode").get_to(t.mode);
    j.at("is_tradable").get_to(t.is_tradable);
    j.at("last_price").get_to(t.last_price);
    j.at("average_trade_price").get_to(t.average_trade_price);
    j.at("net_change").get_to(t.net_change);
    j.at("ohlc").get_to(t.ohlc);
    if (j.contains("market_depth")) {
        j.at("market_depth").get_to(t.market_depth);
    }
}

void from_json(const json& j, postback& p) {
    j.at("quantity").get_to(p.quantity);
    j.at("filled_quantity").get_to(p.filled_quantity);
    j.at("unfilled_quantity").get_to(p.unfilled_quantity);
    j.at("average_price").get_to(p.average_price);
    j.at("price").get_to(p.price);
    j.at("trigger_price").get_to(p.trigger_price);
    j.at("order_id").get_to(p.order_id);
    j.at("exchange_order_id").get_to(p.exchange_order_id);
    j.at("placed_by").get_to(p.placed_by);
    j.at("status").get_to(p.status);
    j.at("status_message").get_to(p.status_message);
    j.at("tradingsymbol").get_to(p.tradingsymbol);
    j.at("exchange").get_to(p.exchange);
    j.at("order_type").get_to(p.order_type);
    j.at("transaction_type").get_to(p.transaction_type);
    j.at("validity").get_to(p.validity);
    j.at("product").get_to(p.product);
    j.at("user_id").get_to(p.user_id);
    j.at("order_timestamp").get_to(p.order_timestamp);
    j.at("exchange_timestamp").get_to(p.exchange_timestamp);
    j.at("checksum").get_to(p.checksum);
}

} // namespace cppkiteconnect
