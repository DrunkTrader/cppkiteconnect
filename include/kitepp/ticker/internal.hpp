/*
 *  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 *  SPDX-License-Identifier: MIT
 *
 *  Copyright (c) 2020-2022 Bhumit Attarde
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

#include <algorithm> //reverse
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring> //memcpy
#include <functional>
#include <ios>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../exceptions.hpp"
#include "../responses/responses.hpp"
#include "../userconstants.hpp" //modes
#include "../utils.hpp"
#include "ws.hpp"
#include "session.hpp"

#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/rapidjson.h"
#include "rapidjson/include/rapidjson/writer.h"

namespace kiteconnect {
// To make sure doubles are parsed correctly
static_assert(std::numeric_limits<double>::is_iec559,
    "Requires IEEE 754 floating point!");
using std::string;
namespace rj = rapidjson;
namespace kc = kiteconnect;
namespace utils = kc::internal::utils;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
inline ticker::ticker(string Key, unsigned int ConnectTimeout,
    bool EnableReconnect, unsigned int maxreconnectdelay,
    unsigned int MaxReconnectTries)
    : ticker(std::move(Key), tickerOptions {}, ConnectTimeout, EnableReconnect,
          maxreconnectdelay, MaxReconnectTries) {}

inline ticker::ticker(string Key, tickerOptions Options,
    unsigned int ConnectTimeout, bool EnableReconnect,
    unsigned int MaxReconnectDelay, unsigned int MaxReconnectTries)
    : key(std::move(Key)), options(std::move(Options)),
      connectTimeout(ConnectTimeout <= std::numeric_limits<int>::max() / 1000 ?
          ConnectTimeout * 1000 : 0), enableReconnect(EnableReconnect),
      maxReconnectDelay(MaxReconnectDelay), maxReconnectTries(MaxReconnectTries) {
    if (!connectTimeout || !maxReconnectDelay || options.host.empty() ||
        options.port.empty() || options.closeTimeout.count() <= 0 ||
        !options.maxMessageBytes || !options.maxQueuedBytes) {
        throw libException("invalid ticker configuration");
    }
}

inline ticker::~ticker() noexcept {
    // The caller must join run() before destruction and must not delete from a callback.
    stopRequested = true;
    retryTimer.cancel();
    if (session) { session->abort(); }
    loop.restart();
    while (loop.poll()) {}
    session.reset();
}

inline void ticker::requireOwner() const {
    const std::lock_guard<std::mutex> lock(ownerMutex);
    if (running && owner != std::this_thread::get_id()) {
        throw libException("ticker operation requires the run() owner thread");
    }
}

inline void ticker::setApiKey(const string& Key) { requireOwner(); key = Key; };

inline string ticker::getApiKey() const { requireOwner(); return key; };

inline void ticker::setAccessToken(const string& Token) { requireOwner(); token = Token; };

inline string ticker::getAccessToken() const { requireOwner(); return token; };

inline void ticker::connect() {
    requireOwner();
    if (state != State::Ready || stopRequested) {
        throw libException("ticker connect already requested or stopped");
    }
    state = State::Connecting;
    boost::asio::post(loop, [this] { if (!stopRequested) { connectInternal(); } });
};

inline bool ticker::isConnected() const {
    return !stopRequested.load() && connected.load();
};

inline std::chrono::time_point<std::chrono::system_clock> ticker::
    getLastBeatTime() const {
    return std::chrono::system_clock::time_point(
        std::chrono::system_clock::duration(heartbeatCount.load()));
};

inline void ticker::run() {
    {
        const std::lock_guard<std::mutex> lock(ownerMutex);
        if (running.exchange(true)) { throw libException("ticker loop is already running"); }
        owner = std::this_thread::get_id();
    }
    loop.restart();
    try { loop.run(); }
    catch (...) { running = false; throw; }
    connected = false;
    state = State::Stopped;
    running = false;
};

inline void ticker::stop() {
    if (stopRequested.exchange(true)) { return; }
    connected = false;
    boost::asio::post(loop, [this] { stopInternal(); });
};

inline void ticker::stopInternal() {
    state = State::Stopping;
    connected = false;
    retryTimer.cancel();
    if (session) { session->shutdown(); }
    state = State::Stopped;
}

inline void ticker::reportError(int code, const string& message) noexcept {
    try {
        const auto callback = onError;
        if (callback) { callback(this, code, message); }
    } catch (...) { stop(); }
}

inline void ticker::validateTokens(const std::vector<int>& tokens) const {
    requireOwner();
    if (tokens.size() > 3000 || std::any_of(tokens.begin(), tokens.end(),
            [](int token) { return token <= 0; })) {
        throw libException("invalid instrument token list");
    }
}

inline void ticker::sendText(string message) {
    requireOwner();
    if (!isConnected() || !session) { throw libException("not connected to websocket server"); }
    session->send(std::move(message));
}

inline void ticker::subscribe(const std::vector<int>& instrumentTokens) {
    validateTokens(instrumentTokens);
    auto desired = subbedInstruments;
    for (int token : instrumentTokens) { desired.emplace(token, DEFAULT_MODE); }
    if (desired.size() > 3000) { throw libException("subscription limit exceeded"); }
    utils::json::json<utils::json::JsonObject> req;
    req.field("a", "subscribe");
    req.field("v", instrumentTokens);
    string reqStr = req.serialize();

    sendText(std::move(reqStr));
    subbedInstruments = std::move(desired);
};

inline void ticker::unsubscribe(const std::vector<int>& instrumentTokens) {
    validateTokens(instrumentTokens);
    utils::json::json<utils::json::JsonObject> req;
    req.field("a", "unsubscribe");
    req.field("v", instrumentTokens);
    string reqStr = req.serialize();

    sendText(std::move(reqStr));
        for (const int tok : instrumentTokens) {
            auto it = subbedInstruments.find(tok);
            if (it != subbedInstruments.end()) { subbedInstruments.erase(it); };
        };
};

inline void ticker::setMode(
    const string& mode, const std::vector<int>& instrumentTokens) {
    validateTokens(instrumentTokens);
    if (mode != MODE_LTP && mode != MODE_QUOTE && mode != MODE_FULL) {
        throw libException("invalid subscription mode");
    }
    for (int token : instrumentTokens) {
        if (!subbedInstruments.count(token)) {
            throw libException("setMode requires subscribed instruments");
        }
    }
    // create request json
    rj::Document req;
    req.SetObject();
    auto& reqAlloc = req.GetAllocator();
    rj::Value val;
    rj::Value valArr(rj::kArrayType);
    rj::Value toksArr(rj::kArrayType);

    val.SetString("mode", reqAlloc);
    req.AddMember("a", val, reqAlloc);

    val.SetString(mode.c_str(), mode.size(), reqAlloc);
    valArr.PushBack(val, reqAlloc);
    for (const int tok : instrumentTokens) { toksArr.PushBack(tok, reqAlloc); }
    valArr.PushBack(toksArr, reqAlloc);
    req.AddMember("v", valArr, reqAlloc);

    // send the request
    string reqStr = utils::json::serialize(req);
    sendText(std::move(reqStr));
        for (const int tok : instrumentTokens) {
            if (mode == MODE_LTP) {
                subbedInstruments[tok] = MODES::LTP;
            } else if (mode == MODE_QUOTE) {
                subbedInstruments[tok] = MODES::QUOTE;
            } else {
                subbedInstruments[tok] = MODES::FULL;
            }
        };
};

inline void ticker::connectInternal() {
    if (stopRequested) { return; }
    state = State::Connecting;
    heartbeatCount = 0;
    try {
        session = std::make_shared<Session>(*this);
        session->start();
    } catch (const std::exception&) {
        reportError(-1, "unable to configure streaming TLS");
        invoke(onConnectError);
        state = State::Stopped;
    }
};

inline void ticker::reconnect() {
    if (stopRequested || isConnected()) { return; }
    if (reconnectTries >= maxReconnectTries) {
        state = State::Stopped;
        invoke(onReconnectFail);
        return;
    }
    state = State::Backoff;
    const auto delay = std::min(reconnectDelay, maxReconnectDelay);
    reconnectDelay = delay > maxReconnectDelay / 2 ? maxReconnectDelay : delay * 2;
    retryTimer.expires_after(std::chrono::seconds(delay));
    retryTimer.async_wait([this](boost::system::error_code error) {
        if (error || stopRequested) { return; }
        ++reconnectTries;
        invoke(onTryReconnect, reconnectTries);
        if (!stopRequested) { connectInternal(); }
    });
};

inline void ticker::processTextMessage(const string& message) {
    rj::Document res;
    utils::json::parse(res, message);
    if (!res.IsObject()) { throw libException("Expected a JSON object"); };

    auto type = utils::json::get<string>(res, "type");
    if (type.empty()) {
        throw kc::libException(
            FMT("Cannot recognize websocket message type {0}", type));
    }

    if (type == "order") {
        const auto postback = kc::postback(utils::json::extractObject(res));
        invoke(onOrderUpdate, postback);
    }
    if (type == "message") {
        utils::json::extractString(res);
        invoke(onMessage, message);
    };
    if (type == "error") {
        reportError(0, utils::json::extractString(res));
    };
};

template <typename T>
#if KITEPP_CPLUSPLUS >= 202002L
    requires(std::integral<T> && !std::same_as<T, bool>)
#endif
T ticker::unpack(const BinaryView& bytes, size_t start, size_t end) {
    if (start > end || end >= bytes.size() || end - start + 1 != sizeof(T)) {
        throw libException("truncated binary field");
    }
    T value;
#if KITEPP_CPLUSPLUS >= 202002L
    static_assert(std::endian::native == std::endian::little ||
        std::endian::native == std::endian::big, "mixed-endian platforms are unsupported");
    if constexpr (std::endian::native == std::endian::big) {
        std::memcpy(&value, bytes.data() + start, sizeof(T));
    } else {
        std::array<char, sizeof(T)> field {};
        for (size_t index = 0; index < sizeof(T); ++index) {
            field[sizeof(T) - 1 - index] = bytes[start + index];
        }
        std::memcpy(&value, field.data(), sizeof(T));
    }
#else
    // Preserve the protected C++17 decoder for rollback/parity verification.
    std::vector<char> requiredBytes(sizeof(T));
    for (size_t index = 0; index < sizeof(T); ++index) {
#ifdef WORDS_BIGENDIAN
        requiredBytes[index] = bytes[start + index];
#else
        requiredBytes[sizeof(T) - 1 - index] = bytes[start + index];
#endif
    }

    std::memcpy(&value, requiredBytes.data(), sizeof(T));
#endif
    return value;
};

inline std::vector<ticker::BinaryView> ticker::splitPackets(
    const BinaryView& bytes) {
    const auto numberOfPackets = unpack<uint16_t>(bytes, 0, 1);
    std::vector<BinaryView> packets;

    size_t packetLengthStartIdx = 2;
    for (int i = 1; i <= numberOfPackets; i++) {
        size_t packetLengthEndIdx = packetLengthStartIdx + 1;
        auto packetLength =
            unpack<uint16_t>(bytes, packetLengthStartIdx, packetLengthEndIdx);
        if (packetLength > bytes.size() - packetLengthEndIdx - 1) {
            throw libException("truncated binary packet");
        }
        if (packetLength != 8 && packetLength != 28 && packetLength != 32 &&
            packetLength != 44 && packetLength != 184) {
            throw libException("unrecognized binary packet size");
        }
        packetLengthStartIdx = packetLengthEndIdx + packetLength + 1;
#if KITEPP_CPLUSPLUS >= 202002L
        packets.emplace_back(bytes.subspan(packetLengthEndIdx + 1, packetLength));
#else
        packets.emplace_back(bytes.begin() + packetLengthEndIdx + 1,
            bytes.begin() + packetLengthStartIdx);
#endif
    };
    if (packetLengthStartIdx != bytes.size()) {
        throw libException("unexpected trailing binary data");
    }
    return packets;
};

inline std::vector<kc::tick> ticker::parseBinaryMessage(
    char* bytes, size_t size) {
    if (!bytes || size < 2 || size > options.maxMessageBytes) {
        throw libException("invalid binary message length");
    }
    static constexpr uint8_t SEGMENT_MASK = 0xff;
    static constexpr double CDS_DIVISOR = 10000000.0;
    static constexpr double BSECDS_DIVISOR = 10000.0;
    static constexpr double GENERIC_DIVISOR = 100.0;
    static constexpr size_t LTP_PACKET_SIZE = 8;
    static constexpr size_t INDICES_QUOTE_PACKET_SIZE = 28;
    static constexpr size_t INDICES_FULL_PACKET_SIZE = 32;
    static constexpr size_t QUOTE_PACKET_SIZE = 44;
    static constexpr size_t FULL_PACKET_SIZE = 184;

#if KITEPP_CPLUSPLUS >= 202002L
    const BinaryView frame(bytes, size);
#else
    const BinaryView frame(bytes, bytes + size);
#endif
    const auto packets = splitPackets(frame);
    if (packets.empty()) { return {}; };

    std::vector<kc::tick> ticks;
    for (const auto& packet : packets) {
        const size_t packetSize = packet.size();
        const auto instrumentToken = unpack<int32_t>(packet, 0, 3);
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        const uint8_t segment = instrumentToken & SEGMENT_MASK;
        const bool tradable =
            segment != static_cast<uint8_t>(SEGMENTS::INDICES);
        double divisor = 0.0;
        if (segment == static_cast<uint8_t>(SEGMENTS::CDS)) {
            divisor = CDS_DIVISOR;

        } else if (segment == static_cast<uint8_t>(SEGMENTS::BSECDS)) {
            divisor = BSECDS_DIVISOR;

        } else {
            divisor = GENERIC_DIVISOR;
        }

        kc::tick Tick;
        Tick.isTradable = tradable;
        Tick.instrumentToken = instrumentToken;

        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
        // LTP packet
        if (packetSize == LTP_PACKET_SIZE) {
            Tick.mode = MODE_LTP;
            Tick.lastPrice = unpack<int32_t>(packet, 4, 7) / divisor;
        } else if (packetSize == INDICES_QUOTE_PACKET_SIZE ||
                   packetSize == INDICES_FULL_PACKET_SIZE) {
            // indices quote and full mode
            Tick.mode = (packetSize == INDICES_QUOTE_PACKET_SIZE) ? MODE_QUOTE :
                                                                    MODE_FULL;
            Tick.lastPrice = unpack<int32_t>(packet, 4, 7) / divisor;
            Tick.ohlc.high = unpack<int32_t>(packet, 8, 11) / divisor;
            Tick.ohlc.low = unpack<int32_t>(packet, 12, 15) / divisor;
            Tick.ohlc.open = unpack<int32_t>(packet, 16, 19) / divisor;
            Tick.ohlc.close = unpack<int32_t>(packet, 20, 23) / divisor;
            Tick.netChange = unpack<int32_t>(packet, 24, 27) / divisor;
            if (packetSize == INDICES_FULL_PACKET_SIZE) {
                Tick.timestamp = unpack<int32_t>(packet, 28, 31);
            }
        } else if (packetSize == QUOTE_PACKET_SIZE ||
                   packetSize == FULL_PACKET_SIZE) {
            // Quote and full mode
            Tick.mode =
                (packetSize == QUOTE_PACKET_SIZE) ? MODE_QUOTE : MODE_FULL;
            Tick.lastPrice = unpack<int32_t>(packet, 4, 7) / divisor;
            Tick.lastTradedQuantity = unpack<int32_t>(packet, 8, 11);
            Tick.averageTradePrice = unpack<int32_t>(packet, 12, 15) / divisor;
            Tick.volumeTraded = unpack<int32_t>(packet, 16, 19);
            Tick.totalBuyQuantity = unpack<int32_t>(packet, 20, 23);
            Tick.totalSellQuantity = unpack<int32_t>(packet, 24, 27);
            Tick.ohlc.open = unpack<int32_t>(packet, 28, 31) / divisor;
            Tick.ohlc.high = unpack<int32_t>(packet, 32, 35) / divisor;
            Tick.ohlc.low = unpack<int32_t>(packet, 36, 39) / divisor;
            Tick.ohlc.close = unpack<int32_t>(packet, 40, 43) / divisor;
            Tick.netChange = Tick.ohlc.close == 0 ? 0 :
                (Tick.lastPrice - Tick.ohlc.close) * 100 / Tick.ohlc.close;

            // parse full mode
            if (packetSize == FULL_PACKET_SIZE) {
                Tick.lastTradeTime = unpack<int32_t>(packet, 44, 47);
                Tick.oi = unpack<int32_t>(packet, 48, 51);
                Tick.oiDayHigh = unpack<int32_t>(packet, 52, 55);
                Tick.oiDayLow = unpack<int32_t>(packet, 56, 59);
                Tick.timestamp = unpack<int32_t>(packet, 60, 63);

                unsigned int depthStartIdx = 64;
                for (int i = 0; i <= 9; i++) {
                    kc::depthWS depth;
                    depth.quantity = unpack<int32_t>(
                        packet, depthStartIdx, depthStartIdx + 3);
                    depth.price = unpack<int32_t>(packet, depthStartIdx + 4,
                                      depthStartIdx + 7) /
                                  divisor;
                    depth.orders = unpack<int16_t>(
                        packet, depthStartIdx + 8, depthStartIdx + 9);

                    (i >= 5) ? Tick.marketDepth.sell.emplace_back(depth) :
                               Tick.marketDepth.buy.emplace_back(depth);
                    depthStartIdx = depthStartIdx + 12;
                };
            };
        };
        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
        ticks.emplace_back(Tick);
    };
    return ticks;
};

inline void ticker::resubInstruments() {
    std::vector<int> instruments;
    std::vector<int> ltpInstruments;
    std::vector<int> quoteInstruments;
    std::vector<int> fullInstruments;
    for (const auto& i : subbedInstruments) {
        instruments.push_back(i.first);
        if (i.second == MODES::LTP) { ltpInstruments.push_back(i.first); };
        if (i.second == MODES::QUOTE) { quoteInstruments.push_back(i.first); };
        if (i.second == MODES::FULL) { fullInstruments.push_back(i.first); };
    };

    if (!instruments.empty()) {
        utils::json::json<utils::json::JsonObject> request;
        request.field("a", "subscribe");
        request.field("v", instruments);
        sendText(request.serialize());
    }
    if (!ltpInstruments.empty()) { setMode(MODE_LTP, ltpInstruments); };
    if (!quoteInstruments.empty()) { setMode(MODE_QUOTE, quoteInstruments); };
    if (!fullInstruments.empty()) { setMode(MODE_FULL, fullInstruments); };
};

} // namespace kiteconnect
