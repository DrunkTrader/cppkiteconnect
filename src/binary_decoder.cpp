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

#include "cppkiteconnect/binary_decoder.hpp"

#include <array>
#include <cstring>
#include <stdexcept>
#include <string_view>

namespace cppkiteconnect {

std::string to_string(packet_type type) {
    switch (type) {
        case packet_type::ltp: return "ltp";
        case packet_type::quote: return "quote";
        case packet_type::full: return "full";
        case packet_type::postback: return "postback";
        case packet_type::order_update: return "order_update";
        default: return "unknown";
    }
}

// Helper functions for reading big-endian data
int16_t binary_decoder::read_int16_be(std::span<const uint8_t> data, size_t offset) {
    if (offset + 2 > data.size()) {
        throw std::out_of_range("Buffer too small for int16 read");
    }
    return static_cast<int16_t>((data[offset] << 8) | data[offset + 1]);
}

int32_t binary_decoder::read_int32_be(std::span<const uint8_t> data, size_t offset) {
    if (offset + 4 > data.size()) {
        throw std::out_of_range("Buffer too small for int32 read");
    }
    return static_cast<int32_t>(
        (static_cast<uint32_t>(data[offset]) << 24) |
        (static_cast<uint32_t>(data[offset + 1]) << 16) |
        (static_cast<uint32_t>(data[offset + 2]) << 8) |
        static_cast<uint32_t>(data[offset + 3])
    );
}

double binary_decoder::read_double_be(std::span<const uint8_t> data, size_t offset) {
    if (offset + 8 > data.size()) {
        throw std::out_of_range("Buffer too small for double read");
    }
    
    // Read as big-endian IEEE 754 double
    uint64_t bits = 0;
    for (int i = 0; i < 8; ++i) {
        bits = (bits << 8) | data[offset + i];
    }
    
    double result;
    std::memcpy(&result, &bits, sizeof(double));
    return result;
}

decoded_ltp binary_decoder::decode_ltp(std::span<const uint8_t> data) {
    // LTP packet: 4 bytes token + 4 bytes price = 8 bytes
    constexpr size_t ltp_size = 8;
    if (data.size() < ltp_size) {
        throw std::invalid_argument("LTP packet too small");
    }
    
    decoded_ltp result;
    result.instrument_token = read_int32_be(data, 0);
    result.last_price = read_double_be(data, 4);
    return result;
}

decoded_quote binary_decoder::decode_quote(std::span<const uint8_t> data) {
    // Quote packet: 28 bytes
    constexpr size_t quote_size = 28;
    if (data.size() < quote_size) {
        throw std::invalid_argument("Quote packet too small");
    }
    
    decoded_quote result;
    result.instrument_token = read_int32_be(data, 0);
    result.last_price = read_double_be(data, 4);
    result.last_traded_quantity = read_int32_be(data, 8);
    result.average_trade_price = read_double_be(data, 12);
    result.volume_traded = read_int32_be(data, 16);
    result.ohlc_open = read_double_be(data, 20);
    result.ohlc_high = read_double_be(data, 24);
    // Note: Simplified - actual protocol has more fields
    result.ohlc_low = 0.0;
    result.ohlc_close = 0.0;
    result.oi = 0;
    return result;
}

decoded_full binary_decoder::decode_full(std::span<const uint8_t> data) {
    // Full packet: 184 bytes
    constexpr size_t full_size = 184;
    if (data.size() < full_size) {
        throw std::invalid_argument("Full packet too small");
    }
    
    decoded_full result;
    size_t offset = 0;
    
    // Basic fields (same as quote plus more)
    result.instrument_token = read_int32_be(data, offset); offset += 4;
    result.last_price = read_double_be(data, offset); offset += 8;
    result.last_traded_quantity = read_int32_be(data, offset); offset += 4;
    result.average_trade_price = read_double_be(data, offset); offset += 8;
    result.volume_traded = read_int32_be(data, offset); offset += 4;
    result.ohlc_open = read_double_be(data, offset); offset += 8;
    result.ohlc_high = read_double_be(data, offset); offset += 8;
    result.ohlc_low = read_double_be(data, offset); offset += 8;
    result.ohlc_close = read_double_be(data, offset); offset += 8;
    result.oi = read_int32_be(data, offset); offset += 4;
    
    // Additional full mode fields
    result.oi_day_high = read_int32_be(data, offset); offset += 4;
    result.oi_day_low = read_int32_be(data, offset); offset += 4;
    result.total_buy_quantity = read_int32_be(data, offset); offset += 4;
    result.total_sell_quantity = read_int32_be(data, offset); offset += 4;
    result.net_change = read_double_be(data, offset); offset += 8;
    result.timestamp = read_int32_be(data, offset); offset += 4;
    result.last_trade_time = read_int32_be(data, offset); offset += 4;
    
    // Tradable flag
    result.tradable = (data[offset] != 0); offset += 1;
    
    // Skip padding/reserved bytes
    offset += 3;
    
    // Market depth: 5 levels buy + 5 levels sell
    // Each level: 4 bytes quantity + 4 bytes price + 2 bytes orders = 10 bytes
    constexpr size_t depth_levels = 5;
    result.buy_depth.reserve(depth_levels);
    result.sell_depth.reserve(depth_levels);
    
    // Buy depth
    for (size_t i = 0; i < depth_levels; ++i) {
        depth_entry entry;
        entry.quantity = read_int32_be(data, offset); offset += 4;
        entry.price = read_double_be(data, offset); offset += 8;
        entry.orders = read_int16_be(data, offset); offset += 2;
        result.buy_depth.push_back(entry);
    }
    
    // Sell depth
    for (size_t i = 0; i < depth_levels; ++i) {
        depth_entry entry;
        entry.quantity = read_int32_be(data, offset); offset += 4;
        entry.price = read_double_be(data, offset); offset += 8;
        entry.orders = read_int16_be(data, offset); offset += 2;
        result.sell_depth.push_back(entry);
    }
    
    return result;
}

std::vector<tick> binary_decoder::decode(std::span<const uint8_t> data) {
    std::vector<tick> result;
    
    if (data.empty()) {
        return result;
    }
    
    // First byte is packet type
    const auto type = static_cast<packet_type>(data[0]);
    const auto packet_data = data.subspan(1);
    
    switch (type) {
        case packet_type::ltp: {
            auto decoded = decode_ltp(packet_data);
            tick t;
            t.instrument_token = decoded.instrument_token;
            t.last_price = decoded.last_price;
            t.mode = subscription_mode::ltp;
            result.push_back(std::move(t));
            break;
        }
        
        case packet_type::quote: {
            auto decoded = decode_quote(packet_data);
            tick t;
            t.instrument_token = decoded.instrument_token;
            t.last_price = decoded.last_price;
            t.last_traded_quantity = decoded.last_traded_quantity;
            t.average_trade_price = decoded.average_trade_price;
            t.volume_traded = decoded.volume_traded;
            t.ohlc_open = decoded.ohlc_open;
            t.ohlc_high = decoded.ohlc_high;
            t.ohlc_low = decoded.ohlc_low;
            t.ohlc_close = decoded.ohlc_close;
            t.mode = subscription_mode::quote;
            result.push_back(std::move(t));
            break;
        }
        
        case packet_type::full: {
            auto decoded = decode_full(packet_data);
            tick t;
            t.instrument_token = decoded.instrument_token;
            t.last_price = decoded.last_price;
            t.last_traded_quantity = decoded.last_traded_quantity;
            t.average_trade_price = decoded.average_trade_price;
            t.volume_traded = decoded.volume_traded;
            t.ohlc_open = decoded.ohlc_open;
            t.ohlc_high = decoded.ohlc_high;
            t.ohlc_low = decoded.ohlc_low;
            t.ohlc_close = decoded.ohlc_close;
            t.open_interest = decoded.oi;
            t.oi_day_high = decoded.oi_day_high;
            t.oi_day_low = decoded.oi_day_low;
            t.total_buy_quantity = decoded.total_buy_quantity;
            t.total_sell_quantity = decoded.total_sell_quantity;
            t.net_change = decoded.net_change;
            t.timestamp = decoded.timestamp;
            t.last_trade_time = decoded.last_trade_time;
            t.tradable = decoded.tradable;
            
            // Convert market depth
            std::vector<depth> buy_depth, sell_depth;
            for (const auto& entry : decoded.buy_depth) {
                depth d;
                d.quantity = entry.quantity;
                d.price = entry.price;
                d.orders = entry.orders;
                buy_depth.push_back(d);
            }
            for (const auto& entry : decoded.sell_depth) {
                depth d;
                d.quantity = entry.quantity;
                d.price = entry.price;
                d.orders = entry.orders;
                sell_depth.push_back(d);
            }
            t.market_depth_buy = buy_depth;
            t.market_depth_sell = sell_depth;
            
            t.mode = subscription_mode::full;
            result.push_back(std::move(t));
            break;
        }
        
        case packet_type::postback:
        case packet_type::order_update: {
            // These are text-based, handled separately
            break;
        }
    }
    
    return result;
}

std::vector<tick> binary_decoder::decode_buffer(std::span<const uint8_t> data) {
    std::vector<tick> result;
    size_t offset = 0;
    
    while (offset < data.size()) {
        // Each packet starts with a length prefix (2 bytes, big-endian)
        if (offset + 2 > data.size()) {
            break;
        }
        
        const uint16_t packet_length = static_cast<uint16_t>(
            (data[offset] << 8) | data[offset + 1]
        );
        offset += 2;
        
        if (offset + packet_length > data.size()) {
            break;
        }
        
        const auto packet_data = data.subspan(offset, packet_length);
        auto ticks = decode(packet_data);
        result.insert(result.end(), ticks.begin(), ticks.end());
        
        offset += packet_length;
    }
    
    return result;
}

} // namespace cppkiteconnect
