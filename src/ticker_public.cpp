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

#include "cppkiteconnect/ticker.hpp"
#include "internal/ticker_impl.hpp"

namespace cppkiteconnect {

// Forward declaration implemented in ticker.cpp
class ticker_impl;

ticker::ticker(std::string api_key, websocket_config config)
    : impl_(std::make_unique<ticker_impl>(std::move(api_key), std::move(config))) {}

ticker::~ticker() = default;

ticker::ticker(ticker&&) noexcept = default;
ticker& ticker::operator=(ticker&&) noexcept = default;

void ticker::on_connect(on_connect_callback cb) {
    impl_->on_connect(std::move(cb));
}

void ticker::on_ticks(on_ticks_callback cb) {
    impl_->on_ticks(std::move(cb));
}

void ticker::on_order_update(on_order_update_callback cb) {
    impl_->on_order_update(std::move(cb));
}

void ticker::on_message(on_message_callback cb) {
    impl_->on_message(std::move(cb));
}

void ticker::on_error(on_error_callback cb) {
    impl_->on_error(std::move(cb));
}

void ticker::on_close(on_close_callback cb) {
    impl_->on_close(std::move(cb));
}

void ticker::on_reconnect(on_reconnect_callback cb) {
    impl_->on_reconnect(std::move(cb));
}

void ticker::on_reconnect_fail(on_reconnect_fail_callback cb) {
    impl_->on_reconnect_fail(std::move(cb));
}

void ticker::connect(const std::string& access_token, const std::string& endpoint) {
    impl_->connect(access_token, endpoint);
}

void ticker::disconnect() {
    impl_->disconnect();
}

bool ticker::is_connected() const noexcept {
    return impl_->is_connected();
}

connection_state ticker::get_state() const noexcept {
    return impl_->get_state();
}

void ticker::subscribe(const std::vector<int32_t>& instrument_tokens, 
                       subscription_mode mode) {
    impl_->subscribe(instrument_tokens, mode);
}

void ticker::unsubscribe(const std::vector<int32_t>& instrument_tokens) {
    impl_->unsubscribe(instrument_tokens);
}

void ticker::set_mode(const std::vector<int32_t>& instrument_tokens, 
                      subscription_mode mode) {
    impl_->set_mode(instrument_tokens, mode);
}

std::set<int32_t> ticker::get_subscribed_instruments() const {
    return impl_->get_subscribed_instruments();
}

std::optional<subscription_mode> ticker::get_mode(int32_t instrument_token) const {
    return impl_->get_mode(instrument_token);
}

} // namespace cppkiteconnect
