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
#include <span>
#include <string>
#include <vector>

#include "responses/ws.hpp"

namespace cppkiteconnect {

/// Binary packet types for Kite WebSocket protocol
enum class packet_type : uint8_t {
    ltp = 1,
    quote = 2,
    full = 3,
    postback = 4,
    order_update = 5
};

/// Convert packet_type to string
[[nodiscard]] std::string to_string(packet_type type);

/// Decode LTP packet (8 bytes)
struct decoded_ltp {
    int32_t instrument_token;
    double last_price;
};

/// Decode Quote packet (28 bytes)
struct decoded_quote {
    int32_t instrument_token;
    double last_price;
    int32_t last_traded_quantity;
    double average_trade_price;
    int32_t volume_traded;
    double ohlc_open;
    double ohlc_high;
    double ohlc_low;
    double ohlc_close;
    int32_t oi;
};

/// Depth entry for Full packet
struct depth_entry {
    int32_t quantity;
    double price;
    int16_t orders;
};

/// Decode Full packet (184 bytes)
struct decoded_full {
    int32_t instrument_token;
    double last_price;
    int32_t last_traded_quantity;
    double average_trade_price;
    int32_t volume_traded;
    double ohlc_open;
    double ohlc_high;
    double ohlc_low;
    double ohlc_close;
    int32_t oi;
    int32_t oi_day_high;
    int32_t oi_day_low;
    
    // Market depth
    std::vector<depth_entry> buy_depth;  // 5 levels
    std::vector<depth_entry> sell_depth; // 5 levels
    
    // Additional fields
    bool tradable;
    int32_t total_buy_quantity;
    int32_t total_sell_quantity;
    double net_change;
    int32_t timestamp;
    int32_t last_trade_time;
};

/// Decode Postback/Order Update packet
struct decoded_postback {
    int32_t status_code;
    std::string message;
    // Additional fields parsed from JSON portion
};

/// Binary decoder for Kite WebSocket packets
class binary_decoder {
public:
    /// Decode a single binary packet
    /// @param data Raw packet data
    /// @return Decoded tick data
    static std::vector<tick> decode(std::span<const uint8_t> data);
    
    /// Decode multiple packets from a single buffer
    /// @param data Buffer containing multiple packets
    /// @return Vector of decoded ticks
    static std::vector<tick> decode_buffer(std::span<const uint8_t> data);
    
private:
    /// Decode LTP packet
    static decoded_ltp decode_ltp(std::span<const uint8_t> data);
    
    /// Decode Quote packet
    static decoded_quote decode_quote(std::span<const uint8_t> data);
    
    /// Decode Full packet
    static decoded_full decode_full(std::span<const uint8_t> data);
    
    /// Read big-endian int16
    static int16_t read_int16_be(std::span<const uint8_t> data, size_t offset);
    
    /// Read big-endian int32
    static int32_t read_int32_be(std::span<const uint8_t> data, size_t offset);
    
    /// Read big-endian double (IEEE 754)
    static double read_double_be(std::span<const uint8_t> data, size_t offset);
};

} // namespace cppkiteconnect
