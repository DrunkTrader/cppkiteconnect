/*
 *  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 *  SPDX-License-Identifier: MIT
 */

#include <exception>
#include <string>

#include <gtest/gtest.h>

#include "../utils.hpp"

namespace kiteconnect {
namespace {

namespace utils = internal::utils;
namespace rj = rapidjson;

TEST(kiteProtection, malformedJsonSyntaxIsReported) {
    rj::Document document;
    EXPECT_THROW(utils::json::parse(document, R"({"data":)"), libException);
    EXPECT_THROW(utils::json::parse(document, std::string("{}\0{}", 5)), libException);
    EXPECT_THROW(utils::json::parse(document, std::string("\"\xff\"", 3)), libException);
}

TEST(kiteProtection, largeIntegerIsAcceptedForDouble) {
    rj::Document document;
    utils::json::parse(document, R"({"value":3000000000})");
    EXPECT_DOUBLE_EQ(utils::json::get<double>(document, "value"), 3000000000.0);
}

TEST(kiteProtection, modelParsingPreservesSourceArray) {
    rj::Document document;
    utils::json::parse(document, R"({"net":[{}],"day":[]})");
    positions output(document.GetObject());
    output.parse(document.GetObject());
    EXPECT_EQ(output.net.size(), 1U);
    EXPECT_EQ(document["net"].Size(), 1U);
}

TEST(kiteProtection, subscriptionActionUsesStringLiteral) {
    utils::json::json<utils::json::JsonObject> request;
    request.field("a", "subscribe");
    request.field("v", std::vector<int> { 408065 });

    EXPECT_EQ(request.serialize(), R"({"a":"subscribe","v":[408065]})");
}

TEST(kiteProtection, http200ErrorEnvelopeIsRejected) {
    const utils::http::response response(utils::http::code::OK,
        R"({"status":"error","error_type":"InputException","message":"bad"})");

    EXPECT_FALSE(response);
}

TEST(kiteProtection, bracketOrderEndpointUsesParentId) {
    test::mockKite client;
    EXPECT_CALL(client, sendReq(testing::_, testing::_, testing::_))
        .WillOnce([](const utils::http::endpoint& endpoint,
            const utils::http::Params&, const utils::FmtArgs& args) {
            EXPECT_EQ(endpoint.Path(args), "/orders/bo/child?parent_order_id=parent");
            return utils::http::response(200,
                R"({"status":"success","data":{"order_id":"child"}})");
        });
    EXPECT_EQ(client.cancelOrder("bo", "child", "parent"), "child");
}

TEST(kiteProtection, libExceptionBaseWhatPreservesDetail) {
    const libException exception("detail");
    const std::exception& base = exception;

    EXPECT_STREQ(base.what(), "detail");
}

TEST(kiteProtection, credentialsRemainVisibleThroughPublicAccessors) {
    kite client("old-key");
    client.setAccessToken("token");
    client.setApiKey("new-key");

    EXPECT_EQ(client.getApiKey(), "new-key");
    EXPECT_EQ(client.getAccessToken(), "token");
}

TEST(kiteProtection, sipModificationFixtureUsesSipId) {
    const std::string body = test::readFile(
        "../tests/mock_responses/mf_sip_modify.json");
    rj::Document document;
    utils::json::parse(document, body);
    const auto data = utils::json::extractObject(document);

    ASSERT_TRUE(data.HasMember("sip_id"));
    EXPECT_FALSE(data.HasMember("order_id"));
}

TEST(kiteProtection, diagnosticUsesEachCallsFieldName) {
    rj::Document document;
    utils::json::parse(document, R"({"first":1,"second":"bad"})");
    EXPECT_DOUBLE_EQ(utils::json::get<double>(document, "first"), 1);
    try {
        utils::json::get<double>(document, "second");
        FAIL() << "wrong type accepted";
    } catch (const libException& error) {
        EXPECT_NE(std::string(error.what()).find("second"), std::string::npos);
    }
}

TEST(kiteProtection, invalidEnvelopesAndElementsAreRejected) {
    for (const std::string body : { "[]", "{}", R"({"data":null})",
            R"({"data":1})" }) {
        rj::Document document;
        utils::json::parse(document, body);
        EXPECT_THROW(utils::json::extractObject(document), libException);
        EXPECT_THROW(utils::json::extractArray(document), libException);
    }
    rj::Document document;
    utils::json::parse(document, R"({"net":[1],"day":[]})");
    EXPECT_THROW(positions(document.GetObject()), libException);
}

TEST(kiteProtection, shortCSVAndCandlesAreRejected) {
    EXPECT_THROW(instrument(std::vector<std::string> { "1" }), libException);
    EXPECT_THROW(mfInstrument(std::vector<std::string> { "1" }), libException);
    rj::Document document;
    utils::json::parse(document, R"(["2026-01-01",1])");
    EXPECT_THROW(historicalData(document.GetArray()), libException);
    utils::json::parse(document, R"([null,1,2,3,4,5])");
    EXPECT_THROW(historicalData(document.GetArray()), libException);
    EXPECT_THROW(utils::csvNumber<uint32_t>("4294967296"), libException);
    EXPECT_THROW(utils::csvNumber<int>("12bad"), libException);
    EXPECT_THROW(utils::csvNumber<double>("nan"), libException);
    EXPECT_THROW(utils::csvNumber<double>("1e9999"), libException);
    EXPECT_THROW(utils::csvNumber<double>("1e-9999"), libException);
    EXPECT_THROW(utils::csvNumber<double>("+1"), libException);
    EXPECT_THROW(utils::csvNumber<double>(" 1"), libException);
    EXPECT_THROW(utils::csvNumber<double>("1 "), libException);
    EXPECT_THROW(utils::csvNumber<double>("0x1p0"), libException);
    EXPECT_DOUBLE_EQ(utils::csvNumber<double>(".5"), .5);
    EXPECT_DOUBLE_EQ(utils::csvNumber<double>("-.5"), -.5);
    EXPECT_DOUBLE_EQ(utils::csvNumber<double>("1e+2"), 100);
    EXPECT_DOUBLE_EQ(utils::csvNumber<double>("0e-9999"), 0);
    EXPECT_DOUBLE_EQ(utils::csvNumber<double>("4.9406564584124654e-324"),
        std::numeric_limits<double>::denorm_min());
    EXPECT_THROW(utils::csvNumber<double>("4.9406564584124654e-324junk"),
        libException);
    EXPECT_EQ(utils::csvNumber<double>(
        "1.00000000000000011102230246251565404236316680908203126"),
        std::nextafter(1.0, 2.0));
}

TEST(kiteProtection, csvNumbersDoNotDependOnGlobalLocale) {
    struct CommaPunctuation : std::numpunct<char> {
        char do_decimal_point() const override { return ','; }
    };
    struct RestoreLocale {
        std::locale previous = std::locale();
        ~RestoreLocale() { std::locale::global(previous); }
    } restore;
    std::locale::global(std::locale(restore.previous, new CommaPunctuation));
    EXPECT_DOUBLE_EQ(utils::csvNumber<double>("1.5"), 1.5);
    EXPECT_THROW(utils::csvNumber<double>("1,5"), libException);
}

TEST(kiteProtection, incompleteRequestsFailBeforeTransport) {
    test::mockKite client;
    EXPECT_THROW(client.placeOrder(placeOrderParams {}), libException);
    EXPECT_THROW(client.placeMfSip(placeMfSipParams {}), libException);
    EXPECT_THROW(client.modifyGtt(modifyGttParams {}), libException);
}

TEST(kiteProtection, unlimitedSIPInstallmentsRemainSupported) {
    testing::StrictMock<test::mockKite> client;
    EXPECT_CALL(client, sendReq(testing::_, testing::_, testing::_))
        .WillOnce([](const utils::http::endpoint&, const utils::http::Params& params,
            const utils::FmtArgs&) {
            EXPECT_EQ(params.find("instalments")->second, "-1");
            return utils::http::response(200,
                R"({"status":"success","data":{"sip_id":"synthetic"}})");
        });
    const auto result = client.placeMfSip(placeMfSipParams().Symbol("synthetic")
        .Amount(1000).Frequency("monthly").Installments(-1));
    EXPECT_EQ(result.sipId, "synthetic");
}

TEST(kiteProtection, parseDiagnosticsDoNotIncludeBody) {
    rj::Document document;
    try {
        utils::json::parse(document, "synthetic-secret-invalid-json");
        FAIL() << "malformed input accepted";
    } catch (const libException& error) {
        EXPECT_EQ(std::string(error.what()).find("synthetic-secret"), std::string::npos);
    }
}

TEST(kiteProtection, componentEncoderUsesUnsignedBytes) {
    EXPECT_EQ(utils::encodeURIComponent("\xc3\xa9+&"), "%C3%A9%2B%26");
    EXPECT_EQ(utils::encodeURIComponent("NSE"), "NSE");
}

TEST(kiteProtection, productionRequestPreservesWireHeadersAndFormBody) {
    httplib::Server server;
    std::string authorization, quantity, target;
    server.Post("/orders/regular", [&](const httplib::Request& request,
        httplib::Response& response) {
        authorization = request.get_header_value("Authorization");
        quantity = request.get_param_value("quantity");
        target = request.target;
        response.set_content(R"({"status":"success","data":{"order_id":"synthetic"}})",
            "application/json");
    });
    const int port = server.bind_to_any_port("127.0.0.1");
    ASSERT_GT(port, 0);
    std::thread owner([&] { server.listen_after_bind(); });
    while (!server.is_running()) { std::this_thread::yield(); }
    httplib::Client client("http://127.0.0.1:" + std::to_string(port));
    utils::http::configureClient(client);
    const utils::http::request request { utils::http::METHOD::POST,
        "/orders/regular", "token synthetic:synthetic", { { "quantity", "12" } } };
    const auto response = request.send(client);
    server.stop();
    owner.join();
    EXPECT_TRUE(response);
    EXPECT_EQ(authorization, "token synthetic:synthetic");
    EXPECT_EQ(quantity, "12");
    EXPECT_EQ(target, "/orders/regular");
}

} // namespace
} // namespace kiteconnect
