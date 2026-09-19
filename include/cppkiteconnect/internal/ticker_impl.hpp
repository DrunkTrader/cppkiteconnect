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
#include <unordered_map>
#include <vector>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "ticker.hpp"
#include "binary_decoder.hpp"

namespace cppkiteconnect {

/// Internal WebSocket transport interface
class websocket_transport_impl;

/// Internal implementation of ticker using Boost.Beast
class ticker_impl : public std::enable_shared_from_this<ticker_impl> {
public:
    /// Constructor
    explicit ticker_impl(std::string api_key, websocket_config config);
    
    /// Destructor
    ~ticker_impl();
    
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
    void connect(const std::string& access_token, const std::string& endpoint);
    
    /// Disconnect from server
    void disconnect();
    
    /// Check if connected
    [[nodiscard]] bool is_connected() const noexcept;
    
    /// Get current state
    [[nodiscard]] connection_state get_state() const noexcept;
    
    /// Subscribe to instruments
    void subscribe(const std::vector<int32_t>& tokens, subscription_mode mode);
    
    /// Unsubscribe from instruments
    void unsubscribe(const std::vector<int32_t>& tokens);
    
    /// Set mode for instruments
    void set_mode(const std::vector<int32_t>& tokens, subscription_mode mode);
    
    /// Get subscribed instruments
    [[nodiscard]] std::set<int32_t> get_subscribed_instruments() const;
    
    /// Get mode for instrument
    [[nodiscard]] std::optional<subscription_mode> get_mode(int32_t token) const;

private:
    /// Start reconnection logic
    void start_reconnect();
    
    /// Attempt reconnect
    void attempt_reconnect(std::size_t attempt_count);
    
    /// Restore subscriptions after reconnect
    void restore_subscriptions();
    
    /// Send subscription command
    void send_subscribe_command(const std::vector<int32_t>& tokens, subscription_mode mode);
    
    /// Send unsubscription command
    void send_unsubscribe_command(const std::vector<int32_t>& tokens);
    
    /// Handle incoming binary message
    void handle_binary_message(std::span<const uint8_t> data);
    
    /// Handle incoming text message (postback)
    void handle_text_message(std::string_view data);
    
    /// Dispatch ticks to callback (thread-safe)
    void dispatch_ticks(std::vector<tick> ticks);
    
    /// Run event loop in background thread
    void run_event_loop();
    
    /// Stop event loop
    void stop_event_loop();

private:
    std::string api_key_;
    std::string access_token_;
    std::string endpoint_;
    websocket_config config_;
    
    std::unique_ptr<boost::asio::io_context> io_context_;
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_;
    std::unique_ptr<boost::asio::thread_pool> callback_executor_;
    std::jthread event_loop_thread_;
    
    std::unique_ptr<websocket_transport_impl> transport_;
    
    std::atomic<connection_state> state_{connection_state::disconnected};
    std::atomic<bool> should_stop_{false};
    
    // Subscription tracking
    std::unordered_map<int32_t, subscription_mode> subscriptions_;
    mutable std::mutex subscriptions_mutex_;
    
    // Callbacks
    on_connect_callback on_connect_cb_;
    on_ticks_callback on_ticks_cb_;
    on_order_update_callback on_order_update_cb_;
    on_message_callback on_message_cb_;
    on_error_callback on_error_cb_;
    on_close_callback on_close_cb_;
    on_reconnect_callback on_reconnect_cb_;
    on_reconnect_fail_callback on_reconnect_fail_cb_;
    
    // Reconnect state
    std::size_t reconnect_attempt_count_{0};
    std::chrono::milliseconds current_reconnect_delay_;
};

} // namespace cppkiteconnect
