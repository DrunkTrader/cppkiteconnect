/* SPDX-License-Identifier: MIT */
#include <kitepp/rest.hpp>
#include <gtest/gtest.h>
#include <chrono>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace kiteconnect {
namespace {
namespace utils = internal::utils;

struct WireServer {
    httplib::Server server;
    std::thread owner;
    int port = 0;

    void start() {
        port = server.bind_to_any_port("127.0.0.1");
        if (port <= 0) { throw std::runtime_error("loopback bind failed"); }
        owner = std::thread([this] { server.listen_after_bind(); });
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!server.is_running() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::yield();
        }
        if (!server.is_running()) {
            finish();
            throw std::runtime_error("loopback startup timed out");
        }
    }
    std::string root() const { return "http://127.0.0.1:" + std::to_string(port); }
    void finish() {
        server.stop();
        if (owner.joinable()) { owner.join(); }
    }
    ~WireServer() { finish(); }
};

TEST(kiteWire, methodsHeadersFormsJSONAndEncodedTargets) {
    std::vector<httplib::Request> received;
    std::mutex captureMutex;
    WireServer fixture;
    auto capture = [&](const httplib::Request& request, httplib::Response& response) {
        const std::lock_guard<std::mutex> lock(captureMutex);
        received.push_back(request);
        response.set_content(R"({"status":"success","data":{}})", "application/json");
    };
    fixture.server.Get("/wire", capture);
    fixture.server.Post("/wire", capture);
    fixture.server.Put("/wire", capture);
    fixture.server.Delete("/wire", capture);
    fixture.start();
    httplib::Client client(fixture.root());
    utils::http::configureClient(client);
    client.set_default_headers({ { "X-Kite-Version", "3" } });
    const std::string auth = "token synthetic:synthetic";
    const std::string target =
        "/wire?i=NSE:A%26B%2BC&from=2017-12-15 09:15:00&literal=a+b";
    const utils::http::Params form { { "quantity", "12" }, { "tag", "A&B+C" } };
    EXPECT_TRUE((utils::http::request { utils::http::METHOD::GET, target, auth, {} }.send(client)));
    EXPECT_TRUE((utils::http::request { utils::http::METHOD::POST, "/wire", auth, form }.send(client)));
    EXPECT_TRUE((utils::http::request { utils::http::METHOD::PUT, "/wire", auth, form }.send(client)));
    EXPECT_TRUE((utils::http::request { utils::http::METHOD::DEL, "/wire", auth, {} }.send(client)));
    const std::string json = R"([{"quantity":12,"enabled":true}])";
    EXPECT_TRUE((utils::http::request { utils::http::METHOD::POST, "/wire", auth, {},
        utils::http::CONTENT_TYPE::JSON, utils::http::CONTENT_TYPE::JSON, json }.send(client)));
    EXPECT_TRUE((utils::http::request { utils::http::METHOD::PUT, "/wire", auth, {},
        utils::http::CONTENT_TYPE::JSON, utils::http::CONTENT_TYPE::JSON, json }.send(client)));
    fixture.finish(); // joins server handlers before inspecting captured requests
    ASSERT_EQ(received.size(), 6U);
    for (const auto& request : received) {
        EXPECT_EQ(request.get_header_value("Authorization"), auth);
        EXPECT_EQ(request.get_header_value("X-Kite-Version"), "3");
    }
    EXPECT_EQ(received[0].target,
        "/wire?i=NSE:A%26B%2BC&from=2017-12-15%2009:15:00&literal=a%2Bb");
    EXPECT_EQ(received[0].get_param_value("i"), "NSE:A&B+C");
    EXPECT_EQ(received[0].get_param_value("from"), "2017-12-15 09:15:00");
    EXPECT_EQ(received[0].get_param_value("literal"), "a+b");
    EXPECT_EQ(received[1].method, "POST");
    EXPECT_EQ(received[2].method, "PUT");
    for (size_t index : { 1U, 2U }) {
        EXPECT_EQ(received[index].get_param_value("quantity"), "12");
        EXPECT_EQ(received[index].get_param_value("tag"), "A&B+C");
        EXPECT_EQ(received[index].get_header_value("Content-Type"), "application/x-www-form-urlencoded");
    }
    EXPECT_EQ(received[3].method, "DELETE");
    for (size_t index : { 4U, 5U }) {
        EXPECT_EQ(received[index].body, json);
        EXPECT_EQ(received[index].get_header_value("Content-Type"), "application/json");
    }
}

TEST(kiteWire, errorsAndReadTimeoutRemainClassified) {
    WireServer fixture;
    fixture.server.Get("/error", [](const httplib::Request&, httplib::Response& response) {
        response.status = 400;
        response.set_content(R"({"status":"error","error_type":"InputException","message":"synthetic reject"})", "application/json");
    });
    fixture.server.Get("/plain", [](const httplib::Request&, httplib::Response& response) {
        response.status = 503;
        response.set_content("synthetic-private-body", "text/plain");
    });
    fixture.server.Get("/slow", [](const httplib::Request&, httplib::Response& response) {
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        response.set_content(R"({"status":"success","data":{}})", "application/json");
    });
    fixture.start();
    httplib::Client client(fixture.root());
    utils::http::configureClient(client);
    const auto error = utils::http::request { utils::http::METHOD::GET, "/error", "", {} }.send(client);
    EXPECT_FALSE(error);
    EXPECT_EQ(error.code, 400);
    EXPECT_EQ(error.errorType, "InputException");
    EXPECT_EQ(error.message, "synthetic reject");
    const auto plain = utils::http::request { utils::http::METHOD::GET, "/plain", "", {} }.send(client);
    EXPECT_FALSE(plain);
    EXPECT_EQ(plain.code, 503);
    EXPECT_EQ(plain.message.find("synthetic-private"), std::string::npos);
    client.set_read_timeout(0, 30000);
    EXPECT_THROW((utils::http::request { utils::http::METHOD::GET, "/slow", "", {} }.send(client)), libException);
}
} // namespace
} // namespace kiteconnect
