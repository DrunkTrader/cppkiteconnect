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

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "responses/ws.hpp"

namespace cppkiteconnect {

/// Subscription mode for ticker
enum class subscription_mode {
    quote,      // Quote mode - LTP, OHLC, volume
    ltp,        // Last traded price only
    full        // Full mode - all fields including market depth
};

/// Convert subscription_mode to string
[[nodiscard]] std::string to_string(subscription_mode mode);

/// Convert string to subscription_mode
[[nodiscard]] subscription_mode subscription_mode_from_string(std::string_view s);

/// Configuration for WebSocket connection
struct websocket_config {
    /// Connection timeout
    std::chrono::seconds connection_timeout{7};
    
    /// Ping interval to keep connection alive
    std::chrono::seconds ping_interval{30};
    
    /// Enable auto-reconnect on disconnection
    bool auto_reconnect{true};
    
    /// Maximum reconnect delay
    std::chrono::seconds max_reconnect_delay{60};
    
    /// Maximum reconnect attempts (0 = unlimited)
    std::size_t max_reconnect_attempts{0};
    
    /// Initial reconnect delay for exponential backoff
    std::chrono::milliseconds initial_reconnect_delay{100};
    
    /// Backoff multiplier for reconnect delay
    double reconnect_backoff_multiplier{2.0};
};

/// Connection state for WebSocket
enum class connection_state {
    disconnected,
    connecting,
    connected,
    reconnecting,
    stopping
};

/// Convert connection_state to string
[[nodiscard]] std::string to_string(connection_state state);

/// Callback types for ticker events
using on_connect_callback = std::function<void()>;
using on_ticks_callback = std::function<void(const std::vector<tick>&)>;
using on_order_update_callback = std::function<void(const postback&)>;
using on_message_callback = std::function<void(const std::string&)>;
using on_error_callback = std::function<void(int code, const std::string& message)>;
using on_close_callback = std::function<void(int code, const std::string& message)>;
using on_reconnect_callback = std::function<void(std::size_t attempt_count)>;
using on_reconnect_fail_callback = std::function<void()>;

/// Forward declaration
class ticker_impl;

/// Modern WebSocket ticker client using Boost.Beast
/// 
/// Features:
/// - Async connect/disconnect with C++20 coroutines
/// - Automatic reconnection with exponential backoff
/// - Subscription tracking and restoration
/// - Thread-safe callback dispatch
/// - Binary packet decoding
/// - Postback support
class ticker {
public:
    /// Construct ticker with API key
    explicit ticker(std::string api_key, websocket_config config = {});
    
    /// Destructor - ensures clean shutdown
    ~ticker();
    
    // Non-copyable, movable
    ticker(const ticker&) = delete;
    ticker& operator=(const ticker&) = delete;
    ticker(ticker&&) noexcept;
    ticker& operator=(ticker&&) noexcept;
    
    /// Set callbacks
    void on_connect(on_connect_callback cb);
    void on_ticks(on_ticks_callback cb);
    void on_order_update(on_order_update_callback cb);
    void on_message(on_message_callback cb);
    void on_error(on_error_callback cb);
    void on_close(on_close_callback cb);
    void on_reconnect(on_reconnect_callback cb);
    void on_reconnect_fail(on_reconnect_fail_callback cb);
    
    /// Connect to WebSocket server
    /// @param access_token User access token
    /// @param endpoint WebSocket endpoint (default: wss://websocket.kite.trade)
    void connect(const std::string& access_token, 
                 const std::string& endpoint = "wss://websocket.kite.trade");
    
    /// Disconnect from WebSocket server
    void disconnect();
    
    /// Check if connected
    [[nodiscard]] bool is_connected() const noexcept;
    
    /// Get current connection state
    [[nodiscard]] connection_state get_state() const noexcept;
    
    /// Subscribe to instruments
    /// @param instrument_tokens List of instrument tokens
    /// @param mode Subscription mode
    void subscribe(const std::vector<int32_t>& instrument_tokens, 
                   subscription_mode mode = subscription_mode::quote);
    
    /// Unsubscribe from instruments
    /// @param instrument_tokens List of instrument tokens
    void unsubscribe(const std::vector<int32_t>& instrument_tokens);
    
    /// Switch subscription mode for instruments
    /// @param instrument_tokens List of instrument tokens
    /// @param mode New subscription mode
    void set_mode(const std::vector<int32_t>& instrument_tokens, subscription_mode mode);
    
    /// Get list of subscribed instruments
    [[nodiscard]] std::set<int32_t> get_subscribed_instruments() const;
    
    /// Get subscription mode for an instrument
    [[nodiscard]] std::optional<subscription_mode> get_mode(int32_t instrument_token) const;

private:
    std::unique_ptr<ticker_impl> impl_;
};

} // namespace cppkiteconnect