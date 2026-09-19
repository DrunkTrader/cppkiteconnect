#include <gtest/gtest.h>
#include "mock_http_client.hpp"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/json.hpp>

namespace cppkiteconnect {

class MockHttpClientTest : public ::testing::Test {
protected:
    boost::asio::io_context io_ctx;
};

TEST_F(MockHttpClientTest, GetRequest_TracksCall) {
    mock_http_client client;
    
    auto result = co_await client.get("/test", {}, {});
    
    EXPECT_EQ(client.request_count(), 1);
    EXPECT_EQ(client.last_method(), "GET");
    EXPECT_EQ(client.last_path(), "/test");
}

TEST_F(MockHttpClientTest, PostRequest_TracksCallAndBody) {
    mock_http_client client;
    std::string body = R"({"order_id": "123"})";
    
    auto result = co_await client.post("/orders", body, {}, {});
    
    EXPECT_EQ(client.request_count(), 1);
    EXPECT_EQ(client.last_method(), "POST");
    EXPECT_EQ(client.last_body(), body);
}

TEST_F(MockHttpClientTest, PutRequest_TracksCallAndBody) {
    mock_http_client client;
    std::string body = R"({"status": "modified"})";
    
    auto result = co_await client.put("/orders/123", body, {}, {});
    
    EXPECT_EQ(client.request_count(), 1);
    EXPECT_EQ(client.last_method(), "PUT");
    EXPECT_EQ(client.last_body(), body);
}

TEST_F(MockHttpClientTest, DeleteRequest_TracksCall) {
    mock_http_client client;
    
    auto result = co_await client.del("/orders/123", {}, {});
    
    EXPECT_EQ(client.request_count(), 1);
    EXPECT_EQ(client.last_method(), "DELETE");
    EXPECT_EQ(client.last_path(), "/orders/123");
}

TEST_F(MockHttpClientTest, SetNextResponse_ReturnsCustomResponse) {
    mock_http_client client;
    mock_http_client::mock_response response{404, R"({"error": "not found"})", {{"content-type", "application/json"}}};
    client.set_next_response(response);
    
    auto result = co_await client.get("/test", {}, {});
    
    EXPECT_EQ(result.status_code, 404);
    EXPECT_EQ(result.body, R"({"error": "not found"})");
    EXPECT_EQ(result.headers.at("content-type"), "application/json");
}

TEST_F(MockHttpClientTest, SetResponseFor_GetSpecificPath) {
    mock_http_client client;
    mock_http_client::mock_response response{200, R"({"data": "specific"})", {}};
    client.set_response_for("GET", "/specific", response);
    
    auto result1 = co_await client.get("/specific", {}, {});
    auto result2 = co_await client.get("/other", {}, {});
    
    EXPECT_EQ(result1.status_code, 200);
    EXPECT_EQ(result1.body, R"({"data": "specific"})");
    EXPECT_EQ(result2.status_code, 200); // default
    EXPECT_EQ(result2.body, "{}");
}

TEST_F(MockHttpClientTest, SetNextException_ThrowsException) {
    mock_http_client client;
    auto ex = std::make_exception_ptr(std::runtime_error("test error"));
    client.set_next_exception(ex);
    
    bool threw = false;
    try {
        co_await client.get("/test", {}, {});
    } catch (const std::runtime_error& e) {
        threw = true;
        EXPECT_STREQ(e.what(), "test error");
    }
    
    EXPECT_TRUE(threw);
}

TEST_F(MockHttpClientTest, ResponseResult_JsonParse) {
    http::response_result result{200, R"({"key": "value", "num": 42})", {}};
    
    auto json_opt = result.json();
    ASSERT_TRUE(json_opt.has_value());
    
    auto& json = json_opt.value();
    EXPECT_EQ(json.at("key").as_string(), "value");
    EXPECT_EQ(json.at("num").as_int64(), 42);
}

TEST_F(MockHttpClientTest, ResponseResult_JsonParseEmpty) {
    http::response_result result{200, "", {}};
    
    auto json_opt = result.json();
    EXPECT_FALSE(json_opt.has_value());
}

TEST_F(MockHttpClientTest, ResponseResult_JsonParseInvalid) {
    http::response_result result{200, "invalid json", {}};
    
    auto json_opt = result.json();
    EXPECT_FALSE(json_opt.has_value());
}

TEST_F(MockHttpClientTest, MultipleRequests_IncrementsCount) {
    mock_http_client client;
    
    co_await client.get("/1", {}, {});
    co_await client.post("/2", "", {}, {});
    co_await client.put("/3", "", {}, {});
    co_await client.del("/4", {}, {});
    
    EXPECT_EQ(client.request_count(), 4);
}

} // namespace cppkiteconnect

// Main function for standalone execution
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
