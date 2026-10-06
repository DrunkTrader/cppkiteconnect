/* SPDX-License-Identifier: MIT */
#pragma once

#include "ws.hpp"
#include <iterator>

namespace kiteconnect {
namespace ticker_detail {
namespace net = boost::asio;
namespace beast = boost::beast;
namespace ssl = net::ssl;
namespace websocket = beast::websocket;
using tcp = net::ip::tcp;

} // namespace ticker_detail

struct ticker::Session : std::enable_shared_from_this<ticker::Session> {
    using Error = boost::system::error_code;
    using tcp = ticker_detail::tcp;
    ticker& client;
    boost::asio::ssl::context tls { boost::asio::ssl::context::tls_client };
    tcp::resolver resolver;
    boost::beast::websocket::stream<
        boost::asio::ssl::stream<boost::beast::tcp_stream>, false> socket;
    boost::asio::steady_timer deadline;
    boost::asio::steady_timer writeDeadline;
    boost::beast::flat_buffer buffer;
    boost::beast::http::response<boost::beast::http::string_body> upgrade;
    std::deque<string> writes;
    size_t queuedBytes = 0;
    bool writing = false;
    bool closing = false;
    bool finished = false;
    bool opened = false;
    bool closeNotified = false;

    explicit Session(ticker& owner)
        : client(owner), resolver(owner.loop), socket(owner.loop, tls),
          deadline(owner.loop), writeDeadline(owner.loop),
          buffer(owner.options.maxMessageBytes) {
        tls.set_verify_mode(boost::asio::ssl::verify_peer);
        if (owner.options.caFile.empty()) { tls.set_default_verify_paths(); }
        else { tls.load_verify_file(owner.options.caFile); }
        socket.next_layer().set_verify_mode(boost::asio::ssl::verify_peer);
        socket.next_layer().set_verify_callback(
            boost::asio::ssl::host_name_verification(owner.options.host));
        if (!SSL_set_tlsext_host_name(socket.next_layer().native_handle(),
                owner.options.host.c_str())) {
            throw libException("unable to configure TLS server name");
        }
        socket.read_message_max(owner.options.maxMessageBytes);
    }

    bool active() const {
        return !finished && !client.stopRequested && client.session.get() == this;
    }

    void start() {
        auto self = shared_from_this();
        deadline.expires_after(std::chrono::milliseconds(client.connectTimeout));
        deadline.async_wait([self](Error error) {
            if (!error && self->active()) { self->fail(boost::asio::error::timed_out); }
        });
        resolver.async_resolve(client.options.host, client.options.port,
            [self](Error error, tcp::resolver::results_type addresses) {
                if (!self->active()) { return; }
                if (error) { return self->fail(error); }
                boost::beast::get_lowest_layer(self->socket).async_connect(addresses,
                    [self](Error error, const tcp::endpoint&) {
                        if (!self->active()) { return; }
                        if (error) { return self->fail(error); }
                        self->handshakeTLS();
                    });
            });
    }

    void handshakeTLS() {
        auto self = shared_from_this();
        socket.next_layer().async_handshake(boost::asio::ssl::stream_base::client,
            [self](Error error) {
                if (!self->active()) { return; }
                if (error) { return self->fail(error, false); }
                auto timeout = boost::beast::websocket::stream_base::timeout::suggested(
                    boost::beast::role_type::client);
                timeout.idle_timeout = std::chrono::seconds(15);
                timeout.keep_alive_pings = true;
                self->socket.set_option(timeout);
                const auto& options = self->client.options;
                const string target = "/?api_key=" + internal::utils::encodeURIComponent(self->client.key) +
                    "&access_token=" + internal::utils::encodeURIComponent(self->client.token);
                self->socket.async_handshake(self->upgrade,
                    options.host + ":" + options.port, target,
                    [self](Error error) {
                        if (!self->active()) { return; }
                        if (error) {
                            const auto code = self->upgrade.result_int();
                            return self->fail(error, code != 401 && code != 403);
                        }
                        self->deadline.cancel();
                        self->opened = true;
                        self->client.state = State::Open;
                        self->client.connected = true;
                        self->client.reconnectTries = 0;
                        self->client.reconnectDelay = self->client.initReconnectDelay;
                        try { self->client.resubInstruments(); }
                        catch (const std::exception&) {
                            self->client.reportError(-1, "subscription replay failed");
                            self->client.stop();
                        }
                        if (self->active()) { self->client.invoke(self->client.onConnect); }
                        if (self->active()) { self->read(); }
                    });
            });
    }

    void read() {
        auto self = shared_from_this();
        socket.async_read(buffer, [self](Error error, size_t) {
            if (!self->active()) { return; }
            if (error) { return self->fail(error); }
            string message = boost::beast::buffers_to_string(self->buffer.data());
            self->buffer.consume(self->buffer.size());
            try {
                if (self->socket.got_text()) {
                    self->client.processTextMessage(message);
                } else if (message.size() == 1) {
                    const auto now = std::chrono::system_clock::now();
                    self->client.heartbeatCount = now.time_since_epoch().count();
                } else {
                    const auto ticks = self->client.parseBinaryMessage(message.data(), message.size());
                    self->client.invoke(self->client.onTicks, ticks);
                }
            } catch (const std::exception&) {
                self->client.reportError(-1, "malformed streaming payload");
                self->client.stop();
            }
            if (self->active()) { self->read(); }
        });
    }

    void send(string message) {
        if (!active() || !opened || closing) { throw libException("ticker is not open"); }
        if (message.size() > client.options.maxQueuedBytes - queuedBytes) {
            throw libException("ticker outbound queue is full");
        }
        queuedBytes += message.size();
        writes.push_back(std::move(message));
        if (!writing) { write(); }
    }

    void write() {
        writing = true;
        socket.text(true);
        auto self = shared_from_this();
        writeDeadline.expires_after(std::chrono::milliseconds(client.connectTimeout));
        writeDeadline.async_wait([self](Error error) {
            if (!error && self->active()) { self->fail(boost::asio::error::timed_out); }
        });
        socket.async_write(boost::asio::buffer(writes.front()),
            [self](Error error, size_t) {
                self->writing = false;
                self->writeDeadline.cancel();
                if (self->finished) { return; }
                self->queuedBytes -= self->writes.front().size();
                self->writes.pop_front();
                if (self->closing) { return self->closeSocket(); }
                if (error) { return self->fail(error); }
                if (!self->writes.empty() && self->active()) { self->write(); }
            });
    }

    void abort() noexcept {
        finished = true;
        Error ignored;
        resolver.cancel();
        deadline.cancel(ignored);
        writeDeadline.cancel(ignored);
        // Cancelling TCP alone leaves Beast's handshake/idle timer alive.
        // Explicitly disarm it so run() drains immediately in every state.
        try {
            socket.set_option(boost::beast::websocket::stream_base::timeout {
                boost::beast::websocket::stream_base::none(),
                boost::beast::websocket::stream_base::none(), false });
        } catch (...) {}
        auto& stream = boost::beast::get_lowest_layer(socket);
        stream.socket().cancel(ignored);
        stream.socket().close(ignored);
    }

    void shutdown() {
        if (finished || closing) { return; }
        closing = true;
        resolver.cancel();
        deadline.cancel();
        if (!opened) { abort(); return; }
        if (writing) {
            writes.erase(std::next(writes.begin()), writes.end());
            queuedBytes = writes.front().size();
        } else {
            writes.clear();
            queuedBytes = 0;
        }
        auto self = shared_from_this();
        deadline.expires_after(client.options.closeTimeout);
        deadline.async_wait([self](Error error) {
            if (!error) { self->completeShutdown(boost::asio::error::timed_out); }
        });
        if (!writing) { closeSocket(); }
    }

    void closeSocket() {
        if (finished) { return; }
        auto self = shared_from_this();
        socket.async_close(boost::beast::websocket::close_code::normal,
            [self](Error error) { self->completeShutdown(error); });
    }

    void completeShutdown(Error error) {
        abort();
        if (opened && !closeNotified) {
            closeNotified = true;
            client.invoke(client.onClose, error ? 1006 : 1000, string());
        }
    }

    void fail(Error error, bool retryable = true) {
        if (finished) { return; }
        const bool remoteClose = error == boost::beast::websocket::error::closed;
        const int code = remoteClose && socket.reason().code ?
            static_cast<int>(socket.reason().code) : 1006;
        const bool normal = code == 1000;
        if (code == 1008) { retryable = false; }
        const bool wasOpen = opened;
        abort();
        client.connected = false;
        if (client.stopRequested) { return; }
        // Do not include request URLs, response bodies, or credentials in errors.
        if (!normal) {
            client.reportError(code, wasOpen ? "stream connection closed" : "stream connection failed");
        }
        if (wasOpen) { client.invoke(client.onClose, code, string(socket.reason().reason)); }
        else { client.invoke(client.onConnectError); }
        if (!client.stopRequested && retryable && !normal && client.enableReconnect) {
            client.reconnect();
        } else { client.state = State::Stopped; }
    }
};
} // namespace kiteconnect
