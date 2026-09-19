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

#include "internal/ticker_impl.hpp"

#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/url.hpp>

#include <iostream>
#include <sstream>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;

namespace cppkiteconnect {

// Internal WebSocket transport implementation
class websocket_transport_impl : public std::enable_shared_from_this<websocket_transport_impl> {
public:
    explicit websocket_transport_impl(net::io_context& io_ctx, ssl::context& ssl_ctx)
        : resolver_(io_ctx), stream_(io_ctx, ssl_ctx) {}
    
    void set_on_message(std::function<void(std::span<const uint8_t>)> on_binary_msg,
                       std::function<void(std::string_view)> on_text_msg) {
        on_binary_message_ = std::move(on_binary_msg);
        on_text_message_ = std::move(on_text_msg);
    }
    
    void set_on_close(std::function<void(int, std::string)> on_close) {
        on_close_ = std::move(on_close);
    }
    
    void connect(const std::string& host, const std::string& port, 
                 const std::string& target, std::string access_token) {
        host_ = host;
        port_ = port;
        target_ = target;
        access_token_ = std::move(access_token);
        
        // Resolve
        auto results = resolver_.resolve(host, port);
        
        // Set SNI Hostname
        if (!SSL_set_tlsext_host_name(stream_.native_handle(), host.c_str())) {
            throw std::runtime_error("Failed to set SNI host name");
        }
        
        // Connect
        net::connect(stream_.next_layer(), results.begin(), results.end());
        
        // SSL Handshake
        stream_.handshake(ssl::stream_base::client);
        
        // WebSocket Upgrade
        stream_.control_callback([this](websocket::frame_type kind, beast::string_view payload) {
            if (kind == websocket::frame_type::close) {
                handle_close();
            }
        });
        
        stream_.set_option(websocket::stream_base::decorator(
            [this](websocket::request_type& req) {
                req.set(http::field::authorization, "token " + access_token_);
                req.set(http::field::user_agent, "cppkiteconnect/2.0");
            }));
        
        stream_.handshake(target_, target_ + "?api_key=" + api_key_);
        
        // Start reading
        do_read();
    }
    
    void write_text(std::string_view message) {
        stream_.write(net::buffer(std::string(message)));
    }
    
    void close() {
        beast::error_code ec;
        stream_.close(websocket::close_code::normal, ec);
        if (ec && ec != websocket::error::closed) {
            // Ignore already closed errors
        }
    }
    
    bool is_open() const noexcept {
        return stream_.is_open();
    }

private:
    void do_read() {
        stream_.async_read(buffer_, [this](beast::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                if (ec != websocket::error::closed) {
                    // Handle error
                }
                return;
            }
            
            // Process message
            const auto data = beast::buffer_bytes(buffer_.data());
            if (stream_.got_text()) {
                // Text message (postback)
                std::string_view text(reinterpret_cast<const char*>(data.data()), data.size());
                if (on_text_message_) {
                    on_text_message_(text);
                }
            } else {
                // Binary message (ticks)
                std::vector<uint8_t> binary_data(data.begin(), data.end());
                if (on_binary_message_) {
                    on_binary_message_(std::span<const uint8_t>(binary_data));
                }
            }
            
            buffer_.consume(bytes_transferred);
            do_read();
        });
    }
    
    void handle_close() {
        if (on_close_) {
            on_close_(websocket::close_code::normal, "Server closed connection");
        }
    }

private:
    std::string host_;
    std::string port_;
    std::string target_;
    std::string access_token_;
    std::string api_key_;
    
    net::ip::tcp::resolver resolver_;
    websocket::stream<ssl::stream<net::ip::tcp::socket>> stream_;
    beast::flat_buffer buffer_{8192};
    
    std::function<void(std::span<const uint8_t>)> on_binary_message_;
    std::function<void(std::string_view)> on_text_message_;
    std::function<void(int, std::string)> on_close_;
};

// ticker_impl implementation
ticker_impl::ticker_impl(std::string api_key, websocket_config config)
    : api_key_(std::move(api_key))
    , config_(std::move(config))
    , io_context_(std::make_unique<net::io_context>())
    , work_guard_(std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(
          io_context_->get_executor()))
    , callback_executor_(std::make_unique<net::thread_pool>(2))
{
    // Initialize SSL context
    ssl::context ssl_ctx{ssl::context::tls_client};
    ssl_ctx.set_default_verify_paths();
    ssl_ctx.set_verify_mode(ssl::verify_peer);
    
    transport_ = std::make_unique<websocket_transport_impl>(*io_context_, ssl_ctx);
}

ticker_impl::~ticker_impl() {
    stop_event_loop();
}

void ticker_impl::on_connect(on_connect_callback cb) {
    on_connect_cb_ = std::move(cb);
}

void ticker_impl::on_ticks(on_ticks_callback cb) {
    on_ticks_cb_ = std::move(cb);
}

void ticker_impl::on_order_update(on_order_update_callback cb) {
    on_order_update_cb_ = std::move(cb);
}

void ticker_impl::on_message(on_message_callback cb) {
    on_message_cb_ = std::move(cb);
}

void ticker_impl::on_error(on_error_callback cb) {
    on_error_cb_ = std::move(cb);
}

void ticker_impl::on_close(on_close_callback cb) {
    on_close_cb_ = std::move(cb);
}

void ticker_impl::on_reconnect(on_reconnect_callback cb) {
    on_reconnect_cb_ = std::move(cb);
}

void ticker_impl::on_reconnect_fail(on_reconnect_fail_callback cb) {
    on_reconnect_fail_cb_ = std::move(cb);
}

void ticker_impl::connect(const std::string& access_token, const std::string& endpoint) {
    access_token_ = access_token;
    endpoint_ = endpoint;
    
    state_ = connection_state::connecting;
    
    // Parse endpoint URL
    boost::url url;
    try {
        url = boost::urls::parse_uri(endpoint);
    } catch (...) {
        if (on_error_cb_) {
            on_error_cb_(-1, "Invalid WebSocket endpoint URL");
        }
        return;
    }
    
    std::string host = std::string(url.host());
    std::string port = url.port().empty() ? "443" : std::string(url.port());
    std::string target = std::string(url.path());
    if (url.query().empty()) {
        target += "?api_key=" + api_key_;
    } else {
        target += "&api_key=" + api_key_;
    }
    
    // Set up transport callbacks
    transport_->set_on_message(
        [this](std::span<const uint8_t> data) {
            handle_binary_message(data);
        },
        [this](std::string_view data) {
            handle_text_message(data);
        }
    );
    
    transport_->set_on_close(
        [this](int code, std::string reason) {
            if (state_ == connection_state::stopping) {
                return; // Intentional close
            }
            
            state_ = connection_state::disconnected;
            
            if (on_close_cb_) {
                on_close_cb_(code, reason);
            }
            
            if (config_.auto_reconnect) {
                start_reconnect();
            }
        }
    );
    
    // Run event loop in background thread
    event_loop_thread_ = std::jthread([this, host, port, target](std::stop_token st) {
        run_event_loop();
        
        try {
            transport_->connect(host, port, target, access_token_);
        } catch (const std::exception& e) {
            if (on_error_cb_) {
                on_error_cb_(-1, e.what());
            }
            
            if (config_.auto_reconnect && state_ != connection_state::stopping) {
                start_reconnect();
            }
        }
    });
    
    // Wait a bit for connection to establish
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    if (transport_->is_open()) {
        state_ = connection_state::connected;
        if (on_connect_cb_) {
            on_connect_cb_();
        }
    }
}

void ticker_impl::disconnect() {
    state_ = connection_state::stopping;
    should_stop_ = true;
    
    if (transport_) {
        transport_->close();
    }
    
    stop_event_loop();
    
    state_ = connection_state::disconnected;
}

bool ticker_impl::is_connected() const noexcept {
    return state_ == connection_state::connected && transport_ && transport_->is_open();
}

connection_state ticker_impl::get_state() const noexcept {
    return state_.load();
}

void ticker_impl::subscribe(const std::vector<int32_t>& tokens, subscription_mode mode) {
    if (!is_connected()) {
        return;
    }
    
    // Track subscription
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        for (const auto& token : tokens) {
            subscriptions_[token] = mode;
        }
    }
    
    // Send subscribe command
    send_subscribe_command(tokens, mode);
}

void ticker_impl::unsubscribe(const std::vector<int32_t>& tokens) {
    if (!is_connected()) {
        return;
    }
    
    // Remove from tracking
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        for (const auto& token : tokens) {
            subscriptions_.erase(token);
        }
    }
    
    // Send unsubscribe command
    send_unsubscribe_command(tokens);
}

void ticker_impl::set_mode(const std::vector<int32_t>& tokens, subscription_mode mode) {
    // Unsubscribe first, then subscribe with new mode
    unsubscribe(tokens);
    subscribe(tokens, mode);
}

std::set<int32_t> ticker_impl::get_subscribed_instruments() const {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    std::set<int32_t> result;
    for (const auto& [token, _] : subscriptions_) {
        result.insert(token);
    }
    return result;
}

std::optional<subscription_mode> ticker_impl::get_mode(int32_t token) const {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    auto it = subscriptions_.find(token);
    if (it != subscriptions_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void ticker_impl::start_reconnect() {
    if (should_stop_) {
        return;
    }
    
    state_ = connection_state::reconnecting;
    attempt_reconnect(0);
}

void ticker_impl::attempt_reconnect(std::size_t attempt_count) {
    if (should_stop_) {
        return;
    }
    
    reconnect_attempt_count_ = attempt_count;
    
    // Check max attempts
    if (config_.max_reconnect_attempts > 0 && 
        attempt_count >= config_.max_reconnect_attempts) {
        state_ = connection_state::disconnected;
        if (on_reconnect_fail_cb_) {
            on_reconnect_fail_cb_();
        }
        return;
    }
    
    // Calculate delay with exponential backoff
    auto delay = config_.initial_reconnect_delay;
    for (std::size_t i = 0; i < attempt_count; ++i) {
        delay = std::chrono::milliseconds(
            static_cast<int64_t>(delay.count() * config_.reconnect_backoff_multiplier)
        );
        if (delay > config_.max_reconnect_delay) {
            delay = config_.max_reconnect_delay;
            break;
        }
    }
    
    current_reconnect_delay_ = delay;
    
    // Schedule reconnect
    std::this_thread::sleep_for(delay);
    
    if (should_stop_) {
        return;
    }
    
    if (on_reconnect_cb_) {
        on_reconnect_cb_(attempt_count);
    }
    
    // Try to reconnect
    state_ = connection_state::connecting;
    
    try {
        // Parse endpoint URL
        boost::url url = boost::urls::parse_uri(endpoint_);
        std::string host = std::string(url.host());
        std::string port = url.port().empty() ? "443" : std::string(url.port());
        std::string target = std::string(url.path()) + "?api_key=" + api_key_;
        
        transport_->connect(host, port, target, access_token_);
        
        if (transport_->is_open()) {
            state_ = connection_state::connected;
            reconnect_attempt_count_ = 0;
            
            // Restore subscriptions
            restore_subscriptions();
            
            if (on_connect_cb_) {
                on_connect_cb_();
            }
        }
    } catch (const std::exception& e) {
        if (on_error_cb_) {
            on_error_cb_(-1, e.what());
        }
        
        // Try again
        attempt_reconnect(attempt_count + 1);
    }
}

void ticker_impl::restore_subscriptions() {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    
    // Group by mode
    std::unordered_map<subscription_mode, std::vector<int32_t>> grouped;
    for (const auto& [token, mode] : subscriptions_) {
        grouped[mode].push_back(token);
    }
    
    // Re-subscribe
    for (const auto& [mode, tokens] : grouped) {
        send_subscribe_command(tokens, mode);
    }
}

void ticker_impl::send_subscribe_command(const std::vector<int32_t>& tokens, subscription_mode mode) {
    if (tokens.empty() || !is_connected()) {
        return;
    }
    
    std::ostringstream oss;
    oss << "a";
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) oss << ",";
        oss << tokens[i];
    }
    
    transport_->write_text(oss.str());
}

void ticker_impl::send_unsubscribe_command(const std::vector<int32_t>& tokens) {
    if (tokens.empty() || !is_connected()) {
        return;
    }
    
    std::ostringstream oss;
    oss << "r";
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) oss << ",";
        oss << tokens[i];
    }
    
    transport_->write_text(oss.str());
}

void ticker_impl::handle_binary_message(std::span<const uint8_t> data) {
    try {
        auto ticks = binary_decoder::decode(data);
        dispatch_ticks(std::move(ticks));
    } catch (const std::exception& e) {
        if (on_error_cb_) {
            on_error_cb_(-2, e.what());
        }
    }
}

void ticker_impl::handle_text_message(std::string_view data) {
    // Text messages are postbacks/order updates
    if (on_message_cb_) {
        net::post(*callback_executor_, [this, msg = std::string(data)]() {
            on_message_cb_(msg);
        });
    }
    
    // Try to parse as order update
    if (on_order_update_cb_) {
        // Simple parsing - in production would use JSON parser
        postback pb;
        // TODO: Parse JSON properly
        net::post(*callback_executor_, [this, pb]() mutable {
            on_order_update_cb_(std::move(pb));
        });
    }
}

void ticker_impl::dispatch_ticks(std::vector<tick> ticks) {
    if (!on_ticks_cb_ || ticks.empty()) {
        return;
    }
    
    net::post(*callback_executor_, [this, ticks = std::move(ticks)]() mutable {
        on_ticks_cb_(ticks);
    });
}

void ticker_impl::run_event_loop() {
    try {
        io_context_->run();
    } catch (const std::exception& e) {
        if (on_error_cb_) {
            on_error_cb_(-1, e.what());
        }
    }
}

void ticker_impl::stop_event_loop() {
    should_stop_ = true;
    
    if (work_guard_) {
        work_guard_->reset();
    }
    
    if (io_context_) {
        io_context_->stop();
    }
    
    if (event_loop_thread_.joinable()) {
        event_loop_thread_.request_stop();
        event_loop_thread_.join();
    }
    
    if (callback_executor_) {
        callback_executor_->join();
    }
}

} // namespace cppkiteconnect
