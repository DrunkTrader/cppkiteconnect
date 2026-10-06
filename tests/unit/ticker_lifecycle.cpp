/* SPDX-License-Identifier: MIT */
#include <kitepp/rest.hpp>
#include <kitepp/ticker.hpp>
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>

namespace kiteconnect {
namespace {
namespace net = boost::asio;
namespace ssl = net::ssl;
namespace beast = boost::beast;
namespace ws = beast::websocket;
using tcp = net::ip::tcp;
using Error = boost::system::error_code;
using Stream = ws::stream<ssl::stream<tcp::socket>, false>;

std::string certPath(const std::string& file) {
    return std::string(KITE_TEST_CERT_DIR) + "/" + file;
}

// A loopback server with no credentials or external service access. Every test
// supplies its bounded script; CTest also imposes an overall process timeout.
struct Server {
    net::io_context loop;
    ssl::context tls { ssl::context::tls_server };
    tcp::acceptor acceptor { loop, { tcp::v4(), 0 } };
    std::thread thread;
    std::atomic<int> accepted { 0 };
    std::atomic<bool> failed { false };

    explicit Server(const std::string& certificate = "trusted") {
        tls.use_certificate_chain_file(certPath(certificate + ".pem"));
        tls.use_private_key_file(certPath(certificate + ".key"), ssl::context::pem);
    }
    ~Server() { if (thread.joinable()) { thread.join(); } }

    tickerOptions options() {
        tickerOptions config;
        config.host = "localhost";
        config.port = std::to_string(acceptor.local_endpoint().port());
        config.caFile = certPath("ca.pem");
        config.closeTimeout = std::chrono::milliseconds(100);
        return config;
    }

    template <class Function>
    void start(Function script, int connections = 1) {
        thread = std::thread([this, script, connections] {
            try {
                for (int index = 0; index < connections; ++index) {
                    Stream stream(loop, tls);
                    acceptor.accept(beast::get_lowest_layer(stream));
                    ++accepted;
                    Error error;
                    stream.next_layer().handshake(ssl::stream_base::server, error);
                    if (error) { continue; }
                    stream.accept(error);
                    if (error) { continue; }
                    script(stream, index);
                }
            } catch (...) { failed = true; }
        });
    }
};

TEST(tickerLifecycle, stopBeforeRunIsTerminal) {
    ticker client("synthetic");
    int connects = 0;
    client.onConnect = [&](ticker*) { ++connects; };
    client.connect();
    client.stop();
    client.stop();
    client.run();
    EXPECT_FALSE(client.isConnected());
    EXPECT_EQ(connects, 0);
    EXPECT_THROW(client.connect(), libException);
}

TEST(tickerLifecycle, stopDuringTLSHandshakeIsBounded) {
    net::io_context loop;
    tcp::acceptor acceptor(loop, { tcp::v4(), 0 });
    tickerOptions options;
    options.host = "localhost";
    options.port = std::to_string(acceptor.local_endpoint().port());
    options.caFile = certPath("ca.pem");
    ticker client("synthetic", options, 1);
    std::thread server([&] {
        tcp::socket socket(loop);
        acceptor.accept(socket);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        client.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
    const auto start = std::chrono::steady_clock::now();
    client.connect();
    client.run();
    server.join();
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(1));
    EXPECT_FALSE(client.isConnected());
}

TEST(tickerLifecycle, trustedTLSAndOrderedSubscriptionCommands) {
    Server server;
    std::vector<std::string> commands;
    server.start([&](Stream& stream, int) {
        for (int index = 0; index < 3; ++index) {
            beast::flat_buffer buffer;
            stream.read(buffer);
            commands.push_back(beast::buffers_to_string(buffer.data()));
        }
        stream.binary(true);
        stream.write(net::buffer(std::string(1, '\0')));
        stream.text(true);
        stream.write(net::buffer(std::string(R"({"type":"message","data":"done"})")));
        beast::flat_buffer buffer;
        Error error;
        stream.read(buffer, error); // client stop closes the connection
    });
    ticker client("synthetic", server.options(), 1);
    const auto owner = std::this_thread::get_id();
    client.onConnect = [&](ticker* client) {
        EXPECT_EQ(std::this_thread::get_id(), owner);
        client->subscribe({ 408065 });
        client->setMode(MODE_FULL, { 408065 });
        client->unsubscribe({ 408065 });
        EXPECT_THROW(client->setMode("invalid", { 408065 }), libException);
    };
    client.onMessage = [&](ticker* client, const std::string&) { client->stop(); };
    client.connect();
    client.run();
    server.thread.join();
    EXPECT_FALSE(server.failed);
    ASSERT_EQ(commands.size(), 3U);
    EXPECT_EQ(commands[0], R"({"a":"subscribe","v":[408065]})");
    EXPECT_EQ(commands[1], R"({"a":"mode","v":["full",[408065]]})");
    EXPECT_EQ(commands[2], R"({"a":"unsubscribe","v":[408065]})");
    EXPECT_GT(client.getLastBeatTime().time_since_epoch().count(), 0);
}

void rejectedCertificate(const std::string& certificate, bool useTrust) {
    Server server(certificate);
    server.start([](Stream&, int) {});
    auto options = server.options();
    if (!useTrust) { options.caFile.clear(); }
    ticker client("synthetic", options, 1, true, 1, 1);
    int connected = 0;
    int errors = 0;
    int retries = 0;
    client.onConnect = [&](ticker*) { ++connected; };
    client.onConnectError = [&](ticker*) { ++errors; };
    client.onTryReconnect = [&](ticker*, unsigned) { ++retries; };
    client.connect();
    client.run();
    EXPECT_EQ(connected, 0);
    EXPECT_EQ(errors, 1);
    EXPECT_EQ(retries, 0);
}

TEST(tickerLifecycle, wrongHostnameIsRejected) { rejectedCertificate("wronghost", true); }
TEST(tickerLifecycle, expiredCertificateIsRejected) { rejectedCertificate("expired", true); }
TEST(tickerLifecycle, untrustedCertificateIsRejected) { rejectedCertificate("trusted", false); }

TEST(tickerLifecycle, reconnectReplaysSubscribeBeforeMode) {
    Server server;
    std::vector<std::string> replay;
    server.start([&](Stream& stream, int attempt) {
        for (int index = 0; index < 2; ++index) {
            beast::flat_buffer buffer;
            stream.read(buffer);
            if (attempt == 1) { replay.push_back(beast::buffers_to_string(buffer.data())); }
        }
        Error error;
        if (attempt == 0) { beast::get_lowest_layer(stream).close(error); }
        else {
            stream.text(true);
            stream.write(net::buffer(std::string(R"({"type":"message","data":"done"})")));
            beast::flat_buffer buffer;
            stream.read(buffer, error);
        }
    }, 2);
    ticker client("synthetic", server.options(), 1, true, 1, 2);
    int connects = 0;
    client.onConnect = [&](ticker* client) {
        if (++connects == 1) {
            client->subscribe({ 408065 });
            client->setMode(MODE_LTP, { 408065 });
        }
    };
    client.onMessage = [&](ticker* client, const std::string&) { client->stop(); };
    client.connect();
    client.run();
    server.thread.join();
    EXPECT_FALSE(server.failed);
    EXPECT_EQ(connects, 2);
    ASSERT_EQ(replay.size(), 2U);
    EXPECT_EQ(replay[0], R"({"a":"subscribe","v":[408065]})");
    EXPECT_EQ(replay[1], R"({"a":"mode","v":["ltp",[408065]]})");
}

TEST(tickerLifecycle, callbackExceptionsStopWithoutRecursiveErrors) {
    Server server;
    server.start([](Stream& stream, int) {
        beast::flat_buffer buffer;
        Error error;
        stream.read(buffer, error);
    });
    ticker client("synthetic", server.options(), 1);
    client.onConnect = [](ticker*) { throw std::runtime_error("callback"); };
    int errors = 0;
    client.onError = [&](ticker*, int, const std::string&) {
        ++errors;
        throw std::runtime_error("error callback");
    };
    client.connect();
    client.run();
    EXPECT_EQ(errors, 1);
    EXPECT_FALSE(client.isConnected());
}

TEST(tickerLifecycle, crossThreadStopInterruptsIdleRead) {
    Server server;
    server.start([](Stream& stream, int) {
        beast::flat_buffer buffer;
        Error error;
        stream.read(buffer, error);
    });
    ticker client("synthetic", server.options(), 1);
    client.connect();
    std::thread owner([&] { client.run(); });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!client.isConnected() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_TRUE(client.isConnected());
    EXPECT_THROW(client.subscribe({ 408065 }), libException);
    client.stop();
    owner.join();
    EXPECT_FALSE(client.isConnected());
}

TEST(tickerLifecycle, stopInterruptsBackoffAndPreventsRetry) {
    net::io_context loop;
    tcp::acceptor acceptor(loop, { tcp::v4(), 0 });
    auto port = acceptor.local_endpoint().port();
    acceptor.close();
    tickerOptions options;
    options.host = "localhost";
    options.port = std::to_string(port);
    options.caFile = certPath("ca.pem");
    ticker client("synthetic", options, 1, true, 1, 2);
    std::atomic<int> errors { 0 };
    int retries = 0;
    client.onConnectError = [&](ticker*) { ++errors; };
    client.onTryReconnect = [&](ticker*, unsigned) { ++retries; };
    client.connect();
    std::thread owner([&] { client.run(); });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!errors && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    client.stop();
    owner.join();
    EXPECT_EQ(errors, 1);
    EXPECT_EQ(retries, 0);
}

TEST(tickerLifecycle, exhaustedRetriesNotifyOnce) {
    net::io_context loop;
    tcp::acceptor acceptor(loop, { tcp::v4(), 0 });
    auto port = acceptor.local_endpoint().port();
    acceptor.close();
    tickerOptions options;
    options.host = "localhost";
    options.port = std::to_string(port);
    options.caFile = certPath("ca.pem");
    ticker client("synthetic", options, 1, true, 1, 1);
    int errors = 0, retries = 0, exhausted = 0;
    client.onConnectError = [&](ticker*) { ++errors; };
    client.onTryReconnect = [&](ticker*, unsigned attempt) { ++retries; EXPECT_EQ(attempt, 1U); };
    client.onReconnectFail = [&](ticker*) { ++exhausted; };
    client.connect();
    client.run();
    EXPECT_EQ(errors, 2);
    EXPECT_EQ(retries, 1);
    EXPECT_EQ(exhausted, 1);
}

TEST(tickerLifecycle, normalCloseDoesNotReconnect) {
    Server server;
    server.start([](Stream& stream, int) { stream.close(ws::close_code::normal); });
    ticker client("synthetic", server.options(), 1, true, 1, 1);
    int closes = 0, errors = 0, retries = 0;
    client.onClose = [&](ticker*, int code, const std::string&) { ++closes; EXPECT_EQ(code, 1000); };
    client.onError = [&](ticker*, int, const std::string&) { ++errors; };
    client.onTryReconnect = [&](ticker*, unsigned) { ++retries; };
    client.connect();
    client.run();
    EXPECT_EQ(closes, 1);
    EXPECT_EQ(errors, 0);
    EXPECT_EQ(retries, 0);
}

TEST(restTLS, trustedAndInvalidCertificateChecks) {
    for (const std::string name : { "trusted", "wronghost", "expired", "untrusted" }) {
        const auto certificate = name == "untrusted" ? "trusted" : name;
        httplib::SSLServer server(certPath(certificate + ".pem").c_str(), certPath(certificate + ".key").c_str());
        server.Get("/", [](const httplib::Request&, httplib::Response& response) {
            response.set_content(R"({"status":"success","data":{}})", "application/json");
        });
        const int port = server.bind_to_any_port("127.0.0.1");
        ASSERT_GT(port, 0);
        std::thread owner([&] { server.listen_after_bind(); });
        while (!server.is_running()) { std::this_thread::yield(); }
        httplib::Client client("https://localhost:" + std::to_string(port));
        if (name != "untrusted") { client.set_ca_cert_path(certPath("ca.pem")); }
        client.set_connection_timeout(1);
        auto result = client.Get("/");
        EXPECT_EQ(static_cast<bool>(result), name == "trusted");
        server.stop();
        owner.join();
    }
}

TEST(tickerLifecycle, policyClosePreservesCodeAndDoesNotRetry) {
    Server server;
    server.start([](Stream& stream, int) {
        ws::close_reason reason(ws::close_code::policy_error);
        reason.reason = "synthetic policy";
        stream.close(reason);
    });
    ticker client("synthetic", server.options(), 1, true, 1, 1);
    int closes = 0, retries = 0;
    client.onClose = [&](ticker*, int code, const std::string& reason) {
        ++closes;
        EXPECT_EQ(code, 1008);
        EXPECT_EQ(reason, "synthetic policy");
    };
    client.onTryReconnect = [&](ticker*, unsigned) { ++retries; };
    client.connect();
    client.run();
    EXPECT_EQ(closes, 1);
    EXPECT_EQ(retries, 0);
}

TEST(tickerLifecycle, fragmentedBinaryPayloadCanBeCopiedByCallback) {
    Server server;
    std::string frame(12, '\0');
    frame[1] = 1;
    frame[3] = 8;
    frame[7] = 1;
    frame[11] = 100;
    server.start([&](Stream& stream, int) {
        stream.binary(true);
        stream.write_some(false, net::buffer(frame.data(), 5));
        stream.ping({});
        stream.write_some(true, net::buffer(frame.data() + 5, frame.size() - 5));
        beast::flat_buffer buffer;
        Error error;
        stream.read(buffer, error);
    });
    ticker client("synthetic", server.options(), 1);
    std::vector<tick> owned;
    int closes = 0;
    client.onTicks = [&](ticker* client, const std::vector<tick>& borrowed) {
        owned = borrowed;
        client->stop();
    };
    client.onClose = [&](ticker*, int, const std::string&) { ++closes; };
    client.connect();
    client.run();
    ASSERT_EQ(owned.size(), 1U);
    EXPECT_DOUBLE_EQ(owned.front().lastPrice, 1.0);
    EXPECT_EQ(closes, 1);
}

TEST(tickerLifecycle, queueOverflowDoesNotRecordSubscription) {
    Server server;
    server.start([](Stream& stream, int) {
        beast::flat_buffer buffer;
        Error error;
        stream.read(buffer, error);
    });
    auto options = server.options();
    options.maxQueuedBytes = 8;
    ticker client("synthetic", options, 1);
    client.onConnect = [](ticker* client) {
        EXPECT_THROW(client->subscribe({ 408065 }), libException);
        EXPECT_THROW(client->setMode(MODE_FULL, { 408065 }), libException);
        EXPECT_THROW(client->subscribe({ -1 }), libException);
        client->stop();
    };
    client.connect();
    client.run();
}

TEST(tickerLifecycle, stalledUpgradeCanBeStopped) {
    Server server;
    ticker client("synthetic", server.options(), 1);
    server.thread = std::thread([&] {
        ssl::stream<tcp::socket> stream(server.loop, server.tls);
        server.acceptor.accept(stream.next_layer());
        stream.handshake(ssl::stream_base::server);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        client.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    });
    client.connect();
    client.run();
    EXPECT_FALSE(client.isConnected());
}

TEST(tickerLifecycle, stopDropsQueuedWritesAndBoundsClose) {
    std::atomic<bool> stopping { false };
    Server server;
    server.start([&](Stream&, int) {
        // Keep the peer stalled through command setup, including slower
        // sanitizer builds. Only shutdown belongs to the close deadline.
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (!stopping.load() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    });
    ticker client("synthetic", server.options(), 1);
    int closes = 0;
    std::chrono::steady_clock::time_point stopStarted;
    client.onConnect = [&](ticker* client) {
        for (int index = 0; index < 1000; ++index) { client->subscribe({ 408065 }); }
        stopStarted = std::chrono::steady_clock::now();
        stopping = true;
        client->stop();
        client->stop();
    };
    client.onClose = [&](ticker*, int, const std::string&) { ++closes; };
    client.connect();
    client.run();
    ASSERT_NE(stopStarted, std::chrono::steady_clock::time_point {});
    EXPECT_LT(std::chrono::steady_clock::now() - stopStarted, std::chrono::seconds(1));
    EXPECT_EQ(closes, 1);
    EXPECT_FALSE(client.isConnected());
}

TEST(tickerLifecycle, authorizationFailureDoesNotRetry) {
    Server server;
    ticker client("synthetic", server.options(), 1, true, 1, 1);
    server.thread = std::thread([&] {
        ssl::stream<tcp::socket> stream(server.loop, server.tls);
        server.acceptor.accept(stream.next_layer());
        stream.handshake(ssl::stream_base::server);
        beast::flat_buffer buffer;
        beast::http::request<beast::http::string_body> request;
        beast::http::read(stream, buffer, request);
        beast::http::response<beast::http::empty_body> response {
            beast::http::status::forbidden, 11 };
        response.prepare_payload();
        beast::http::write(stream, response);
    });
    int retries = 0, connects = 0;
    client.onTryReconnect = [&](ticker*, unsigned) { ++retries; };
    client.onConnect = [&](ticker*) { ++connects; };
    client.connect();
    client.run();
    EXPECT_EQ(connects, 0);
    EXPECT_EQ(retries, 0);
}
} // namespace
} // namespace kiteconnect
