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

#include "config.hpp"

#include <cstdint>
#include <cmath>
#include <charconv>
#include <limits>
#include <map>
#include <locale>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

#include "exceptions.hpp"

#include "cpp-httplib/httplib.h"
#include "fmt/include/fmt/args.h"
#include "fmt/include/fmt/format.h"
#include "rapidcsv/src/rapidcsv.h"
#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/encodings.h"
#include "rapidjson/include/rapidjson/rapidjson.h"
#include "rapidjson/include/rapidjson/stringbuffer.h"
#include "rapidjson/include/rapidjson/writer.h"

// Check endieness of platform
#if KITEPP_CPLUSPLUS < 202002L
#if defined(_WIN32)
// Do nothing (Assuming all modern Windows machines are little endian)
#else // Windows check
#ifdef __BIG_ENDIAN__
#define WORDS_BIGENDIAN 1
#else /* __BIG_ENDIAN__ */
#ifdef __LITTLE_ENDIAN__
#undef WORDS_BIGENDIAN
#else
#ifdef BSD
#include <sys/endian.h>
#else
#include <endian.h>
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
#define WORDS_BIGENDIAN 1
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#undef WORDS_BIGENDIAN
#else
#error "unable to determine endianess!"
#endif /* __BYTE_ORDER */
#endif /* __LITTLE_ENDIAN__ */
#endif /* __BIG_ENDIAN__ */
#endif // Windows check
#endif // C++17 compatibility; C++20 decoding uses std::endian

// NOLINTNEXTLINE(google-global-names-in-headers, misc-unused-using-decls)
using fmt::literals::operator""_a;
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define FMT fmt::format

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GENERATE_FLUENT_METHOD(returnType, fieldType, fieldName, methodName)   \
    returnType& methodName(fieldType arg) {                                    \
        (fieldName) = arg;                                                     \
        return *this;                                                          \
    };

namespace kiteconnect::internal::utils {
using std::string;
namespace rj = rapidjson;

template <class Number, class = void>
struct hasFromChars : std::false_type {};
template <class Number>
struct hasFromChars<Number, std::void_t<decltype(std::from_chars(
    std::declval<const char*>(), std::declval<const char*>(),
    std::declval<Number&>()))>> : std::true_type {};

template <class Number>
inline Number csvNumber(const string& value) {
    if (value.empty()) { return Number {}; }
    Number output {};
    if constexpr (hasFromChars<Number>::value) {
        const auto result = std::from_chars(
            value.data(), value.data() + value.size(), output);
        if (result.ec != std::errc {} || result.ptr != value.data() + value.size()) {
            throw libException("invalid or out-of-range CSV number");
        }
    } else {
        // Older libc++ lacks floating from_chars. Classic-locale, no-whitespace
        // extraction keeps the same whole-field/range policy without global
        // locale mutation or an intermediate floating-point rounding step.
        static_assert(std::is_floating_point_v<Number>, "unsupported CSV number type");
        if (value.find_first_not_of("0123456789eE+-.") != string::npos) {
            throw libException("invalid CSV number");
        }
        std::istringstream input(value);
        input.imbue(std::locale::classic());
        input >> std::noskipws >> output;
        // libc++ reports ERANGE even for representable subnormal strtod results.
        // Accept those values, but still reject zero underflow and overflow.
        const bool subnormal = output != 0 && std::isfinite(output) &&
            std::abs(output) < std::numeric_limits<Number>::min();
        if (input.fail() && subnormal) { input.clear(); }
        if (value.front() == '+' || input.fail() ||
            input.peek() != std::char_traits<char>::eof()) {
            throw libException("invalid or out-of-range CSV number");
        }
        const auto mantissa = value.substr(0, value.find_first_of("eE"));
        if (output == 0 && mantissa.find_first_of("123456789") != string::npos) {
            throw libException("out-of-range CSV number");
        }
    }
    if constexpr (std::is_floating_point_v<Number>) {
        if (!std::isfinite(output)) { throw libException("non-finite CSV number"); }
    }
    return output;
}

inline string encodeURIComponent(const string& value) {
    constexpr char hex[] = "0123456789ABCDEF";
    string output;
    for (unsigned char byte : value) {
        if ((byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
            (byte >= '0' && byte <= '9') || byte == '-' || byte == '_' ||
            byte == '.' || byte == '~') {
            output += static_cast<char>(byte);
        } else {
            output += '%';
            output += hex[byte >> 4];
            output += hex[byte & 15];
        }
    }
    return output;
}

//! init_list doesn't have compare operator & gmock needs it
using FmtArgs = std::vector<string>;
template <typename>
struct isOptional : std::false_type {};
template <typename T>
struct isOptional<std::optional<T>> : std::true_type {};

template <typename>
struct isVector : std::false_type {};
template <typename T>
struct isVector<std::vector<T>> : std::true_type {};

constexpr uint16_t MILLISECONDS_IN_A_SECOND = 1000;

namespace json {

using JsonObject = rj::GenericValue<rj::UTF8<>>::Object;
using JsonArray = rj::GenericValue<rj::UTF8<>>::Array;

inline JsonObject checkedObject(rj::Value& value) {
    if (!value.IsObject()) { throw libException("expected JSON object"); }
    return value.GetObject();
}

inline JsonArray checkedArray(rj::Value& value) {
    if (!value.IsArray()) { throw libException("expected JSON array"); }
    return value.GetArray();
}

inline rj::Value& requiredMember(rj::Value& value, const char* name) {
    if (!value.IsObject() || !value.HasMember(name)) {
        throw libException(FMT("missing JSON member: {0}", name));
    }
    return value[name];
}

inline JsonArray memberArray(const JsonObject& object, const char* name) {
    auto member = object.FindMember(name);
    if (member == object.MemberEnd() || !member->value.IsArray()) {
        throw libException(FMT("expected JSON array: {0}", name));
    }
    return member->value.GetArray();
}

template <class Model>
inline std::vector<Model> objectArray(const JsonObject& object,
    const char* name) {
    std::vector<Model> output;
    auto member = object.FindMember(name);
    if (member == object.MemberEnd()) { return output; }
    auto array = checkedArray(member->value);
    output.reserve(array.Size());
    for (auto& value : array) { output.emplace_back(checkedObject(value)); }
    return output;
}
template <class Res>
using CustomObjectParser = std::function<Res(JsonObject&)>;
template <class Res>
using CustomArrayParser = std::function<Res(JsonArray&)>;
template <class Res, class Data, bool UseCustomParser>
using CustomParser = std::conditional_t<std::is_same_v<Data, JsonObject>,
    const CustomObjectParser<Res>&, const CustomArrayParser<Res>&>;
template <class T>
using JsonEncoder = std::function<void(const T&, rj::Value&)>;

// FIXME templatize extract* methods
inline JsonObject extractObject(rj::Document& doc) {
    return checkedObject(requiredMember(doc, "data"));
}

inline JsonArray extractArray(rj::Document& doc) {
    return checkedArray(requiredMember(doc, "data"));
}

inline bool extractBool(rj::Document& doc) {
    auto& data = requiredMember(doc, "data");
    if (!data.IsBool()) { throw libException("expected boolean data"); }
    return data.GetBool();
}

inline string extractString(rj::Document& doc) {
    auto& data = requiredMember(doc, "data");
    if (!data.IsString()) { throw libException("expected string data"); }
    return string(data.GetString(), data.GetStringLength());
}

template <class Output, class Document = rj::Value::Object>
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
inline Output get(const Document& val, const char* name) {
    const auto exceptionString = [name](const string& type) {
        return FMT("type of {0} not is not {1}", name, type);
    };

    if constexpr (std::is_base_of_v<rj::Value, Document>) {
        if (!val.IsObject()) { throw libException("expected JSON object"); }
    }
    auto it = val.FindMember(name);
    if (it != val.MemberEnd()) {
        if constexpr (!isVector<Output>::value) {
            if constexpr (std::is_same_v<std::decay_t<Output>, string>) {
                if (it->value.IsString()) {
                    return string(it->value.GetString(), it->value.GetStringLength());
                };
                if (it->value.IsNull()) { return ""; };
                throw libException(exceptionString("string"));
            } else if constexpr (std::is_same_v<std::decay_t<Output>, double>) {
                if (it->value.IsNumber()) { return it->value.GetDouble(); };
                throw libException(exceptionString("double"));
            } else if constexpr (std::is_same_v<std::decay_t<Output>, int>) {
                if (it->value.IsInt()) { return it->value.GetInt(); };
                throw libException(exceptionString("int"));
            } else if constexpr (std::is_same_v<std::decay_t<Output>,
                                     uint32_t>) {
                if (it->value.IsUint()) { return it->value.GetUint(); };
                throw libException(exceptionString("uint32_t"));
            } else if constexpr (std::is_same_v<std::decay_t<Output>,
                                     int64_t>) {
                if (it->value.IsInt64()) { return it->value.GetInt64(); };
                throw libException(exceptionString("int64_t"));
            } else if constexpr (std::is_same_v<std::decay_t<Output>, bool>) {
                if (it->value.IsBool()) { return it->value.GetBool(); };
                throw libException(exceptionString("bool"));
            } else {
                throw libException("type not supported");
            }
        } else {
            if (it->value.IsArray()) {
                Output out;
                for (const auto& v : it->value.GetArray()) {
                    if constexpr (std::is_same_v<
                                      std::decay_t<typename Output::value_type>,
                                      string>) {
                        (v.IsString()) ?
                            out.emplace_back(v.GetString(), v.GetStringLength()) :
                            throw libException(exceptionString("string"));
                    } else if constexpr (std::is_same_v<
                                             std::decay_t<
                                                 typename Output::value_type>,
                                             double>) {
                        if (v.IsNumber()) {
                            out.emplace_back(v.GetDouble());
                            continue;
                        };
                        throw libException(exceptionString("array of doubles"));
                    } else {
                        throw libException("type not supported");
                    }
                }
                return out;
            };
            throw libException(exceptionString("array"));
        }
    } else {
        return {};
    }
};

template <class Val, class Output>
Output get(const rj::Value::Object& val, const char* name) {
    const auto exceptionString = [name](const string& type) {
        return FMT("type of {0} not is not {1}", name, type);
    };

    auto it = val.FindMember(name);
    if constexpr (std::is_same_v<Val, JsonObject>) {
        static_assert(std::is_constructible_v<Output, rj::Value::Object>);
        rj::Value out(rj::kObjectType);
        if (it != val.MemberEnd()) {
            if (it->value.IsObject()) { return Output(it->value.GetObject()); };
            throw libException(exceptionString("object"));
        };
        return {};
    } else if constexpr (std::is_same_v<Val, JsonArray>) {
        static_assert(std::is_constructible_v<Output, rj::Value::Array>);
        rj::Value out(rj::kArrayType);
        if (it != val.MemberEnd()) {
            if (it->value.IsArray()) { return Output(it->value.GetArray()); };
            throw libException(exceptionString("array"));
        };
        return {};
    };
    return {};
};

template <class Val>
bool get(const rj::Value::Object& val, rj::Value& out, const char* name) {
    const auto exceptionString = [name](const string& type) {
        return FMT("type of {0} not is not {1}", name, type);
    };

    auto it = val.FindMember(name);
    if constexpr (std::is_same_v<Val, JsonObject>) {
        if (it != val.MemberEnd()) {
            if (it->value.IsObject()) {
                out = it->value.GetObject();
                return true;
            };
            throw libException(exceptionString("object"));
        };
        return false;
    } else if constexpr (std::is_same_v<Val, JsonArray>) {
        if (it != val.MemberEnd()) {
            if (it->value.IsArray()) {
                out = it->value.GetArray();
                return true;
            };
            throw libException(exceptionString("array"));
        };
        return false;
    };
    return false;
};

template <class Res, class Data, bool UseCustomParser>
Res parse(
    rj::Document& doc, CustomParser<Res, Data, UseCustomParser> customParser) {
    if constexpr (std::is_same_v<Data, JsonObject>) {
        auto object = extractObject(doc);
        if constexpr (UseCustomParser) {
            return customParser(object);
        } else {
            static_assert(std::is_constructible_v<Res, JsonObject>,
                "Res should be constructable using JsonObject");
            return Res(object);
        }
    } else if constexpr (std::is_same_v<Data, JsonArray>) {
        auto array = extractArray(doc);
        if constexpr (UseCustomParser) {
            return customParser(array);
        } else {
            static_assert(std::is_constructible_v<Res, JsonArray>,
                "Res should be constructable using JsonArray");
            return Res(array);
        }
    } else if constexpr (std::is_same_v<Data, bool>) {
        static_assert(
            std::is_same_v<Res, bool>, "Res needs to be bool if Data is bool");
        return extractBool(doc);
    }
}

inline bool parse(rj::Document& dom, const string& str) {
    if (str.find('\0') != string::npos) {
        throw libException("embedded NUL in JSON input");
    }
    rj::ParseResult result =
        dom.Parse<rj::kParseValidateEncodingFlag>(str.data(), str.size());
    if (result == nullptr) {
        throw libException(FMT("invalid JSON at byte {0} (code {1})",
            result.Offset(), static_cast<unsigned>(result.Code())));
    };
    return true;
};

inline string serialize(rj::Document& dom) {
    rj::StringBuffer buffer;
    rj::Writer<rj::StringBuffer> writer(buffer);
    dom.Accept(writer);
    return buffer.GetString();
}

template <class T>
class json {
  public:
    json() {
        if constexpr (std::is_same_v<T, JsonObject>) {
            dom.SetObject();
        } else {
            dom.SetArray();
        }
    };

    template <class Value>
    void field(const string& name, const Value& value,
        rj::Value* docOverride = nullptr) {
        auto& allocater = dom.GetAllocator();

        if constexpr (std::is_same_v<std::decay_t<Value>, string>) {
            buffer.SetString(value.c_str(), value.size(), allocater);
        } else if constexpr (std::is_convertible_v<const Value&, std::string_view>) {
            const std::string_view text(value);
            buffer.SetString(text.data(), text.size(), allocater);
        } else if constexpr (std::is_same_v<std::decay_t<Value>, bool>) {
            buffer.SetBool(value);
        } else if constexpr (std::is_unsigned_v<std::decay_t<Value>>) {
            buffer.SetUint64(value);
        } else if constexpr (std::is_integral_v<std::decay_t<Value>>) {
            buffer.SetInt64(value);
        } else if constexpr (std::is_floating_point_v<std::decay_t<Value>>) {
            if (!std::isfinite(value)) { throw libException("non-finite JSON number"); }
            buffer.SetDouble(value);
        } else {
            static_assert(!sizeof(Value), "unsupported JSON scalar type");
        };

        if (docOverride == nullptr) {
            dom.AddMember(
                rj::Value(name.c_str(), allocater).Move(), buffer, allocater);
        } else {
            docOverride->AddMember(
                rj::Value(name.c_str(), allocater).Move(), buffer, allocater);
        }
    }

    template <class Value>
    void field(const string& name, const std::vector<Value>& values,
        const JsonEncoder<Value>& encode = {}) {
        auto& allocater = dom.GetAllocator();
        rj::Value arrayBuffer(rj::kArrayType);
        for (const auto& i : values) {
            if constexpr (std::is_fundamental_v<std::decay_t<Value>>) {
                if constexpr (std::is_floating_point_v<Value>) {
                    if (!std::isfinite(i)) { throw libException("non-finite JSON number"); }
                }
                arrayBuffer.PushBack(i, allocater);
            } else {
                rj::Value objectBuffer(rj::kObjectType);
                encode(i, objectBuffer);
                arrayBuffer.PushBack(objectBuffer, allocater);
            }
        };

        if constexpr (std::is_same_v<T, JsonObject>) {
            dom.AddMember(rj::Value(name.c_str(), dom.GetAllocator()).Move(),
                arrayBuffer, allocater);
        } else {
            dom.Swap(arrayBuffer);
        }
    }

    template <class Value>
    void array(const std::vector<Value>& values,
        const JsonEncoder<Value>& encode = {}) {
        field("", values, encode);
    }

    string serialize() {
        rj::StringBuffer strBuffer;
        rj::Writer<rj::StringBuffer> writer(strBuffer);
        dom.Accept(writer);
        return strBuffer.GetString();
    }

  private:
    rj::Document dom;
    rj::Value buffer;
};
} // namespace json

namespace http {

// Preserve the SDK's original parameter type independently of the transport's
// newer insertion-ordered container. Convert only at the HTTP adapter boundary.
using Params = std::multimap<string, string>;

inline void configureClient(httplib::Client& client) {
    // Configure once before use, never mutate client options during a send.
    client.set_path_encode(false);
}

inline string encodeRequestTarget(const string& path) {
    if (path.empty() || path.front() != '/' || path.find('\0') != string::npos) {
        throw libException("invalid HTTP request target");
    }
    constexpr char hex[] = "0123456789ABCDEF";
    string output;
    for (unsigned char byte : path) {
        // Retain the qualified legacy encoding (including literal '+') and
        // already-encoded components. Controls are never emitted on the wire.
        if (byte <= 0x20 || byte >= 0x7f || byte == '+' || byte == '\'' ||
            byte == ',' || byte == ';') {
            output += '%';
            output += hex[byte >> 4];
            output += hex[byte & 15];
        } else {
            output += static_cast<char>(byte);
        }
    }
    return output;
}

namespace code {
constexpr uint16_t OK = 200;
} // namespace code

enum class METHOD : uint8_t
{
    GET,
    POST,
    PUT,
    DEL,
    HEAD
};

enum class CONTENT_TYPE : uint8_t
{
    JSON,
    NON_JSON
};

struct endpoint {
    bool operator==(const endpoint& lhs) const {
        return lhs.method == this->method && lhs.Path.Path == this->Path.Path &&
               lhs.contentType == this->contentType;
    }

    METHOD method = METHOD::GET;
    struct path {

        string operator()(const FmtArgs& fmtArgs = {}) const {
            if (!fmtArgs.empty()) {
                fmt::dynamic_format_arg_store<fmt::format_context> store;
                for (const auto& arg : fmtArgs) { store.push_back(arg); };
                return fmt::vformat(Path, store);
            };
            return Path;
        };

        string Path;
    } Path;
    CONTENT_TYPE contentType = CONTENT_TYPE::NON_JSON;
    CONTENT_TYPE responseType = CONTENT_TYPE::JSON;
};

class response {
  public:
    response(uint16_t Code, const string& body, bool json = true): code(Code) {
        parse(Code, body, json);
    };

    explicit operator bool() const { return !error; };

    uint16_t code = 0;  /// http code
    bool error = false; /// true if kite api reported an error (\a status field)
    rj::Document data;  /// parsed body
    string errorType =
        "NoException"; /// corresponds to kite api's \a error_type field (if \a
                       /// error is \a true)
    string message; /// corresponds to kite api's \a message field (if \a error
                    /// is \a true)
    string rawBody; /// raw body, set in case of non-json response

  private:
    void parse(uint16_t code, const string& body, bool json) {
        if (json) {
            try {
                json::parse(data, body);
            } catch (const libException&) {
                if (code == code::OK) { throw; }
                error = true;
                message = "non-JSON HTTP error response";
                return;
            }
            json::checkedObject(data);
            const auto status = utils::json::get<string, rj::Document>(data, "status");
            if (status != "success" && status != "error" &&
                !(status.empty() && code == code::OK && data.HasMember("data"))) {
                throw libException("missing or invalid response status");
            }
            error = code != code::OK || status == "error";
            if (error) {
                const auto type = utils::json::get<string, rj::Document>(data, "error_type");
                if (!type.empty()) { errorType = type; }
                message = utils::json::get<string, rj::Document>(data, "message");
            }
        } else {
            if (code != static_cast<uint16_t>(code::OK)) { error = true; };
            rawBody = body;
        }
    };
};

struct request {

    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    response send(httplib::Client& client) const {
        // The SDK owns target encoding. Its client is configured once to avoid
        // a second provider normalization step (see configureClient).
        const string target = encodeRequestTarget(path);
        httplib::Params form;
        for (const auto& parameter : body) {
            form.emplace(parameter.first, parameter.second);
        }
        const httplib::Headers headers = { { "Authorization", authToken } };
        uint16_t code = 0;
        string data;

        // httplib::Result doesn't have a default constructor and using a
        // pointer causes segfault
        switch (method) {
            case utils::http::METHOD::GET:
                if (auto res = client.Get(target, headers)) {
                    code = res->status;
                    data = res->body;
                } else {
                    throw libException(FMT("request failed ({0})",
                        httplib::to_string(res.error())));
                }
                break;
            case utils::http::METHOD::POST:
                if (contentType != CONTENT_TYPE::JSON) {
                    if (auto res = client.Post(target, headers, form)) {
                        code = res->status;
                        data = res->body;
                    } else {
                        throw libException(FMT("request failed ({0})",
                            httplib::to_string(res.error())));
                    }
                } else {
                    if (auto res = client.Post(target, headers, serializedBody,
                            "application/json")) {
                        code = res->status;
                        data = res->body;
                    } else {
                        throw libException(FMT("request failed({0})",
                            httplib::to_string(res.error())));
                    }
                };
                break;
            case utils::http::METHOD::PUT:
                if (contentType != CONTENT_TYPE::JSON) {
                    if (auto res = client.Put(target, headers, form)) {
                        code = res->status;
                        data = res->body;
                    } else {
                        throw libException(FMT("request failed ({0})",
                            httplib::to_string(res.error())));
                    }
                } else {
                    if (auto res = client.Put(target, headers, serializedBody,
                            "application/json")) {
                        code = res->status;
                        data = res->body;
                    } else {
                        throw libException(FMT("request failed({0})",
                            httplib::to_string(res.error())));
                    }
                }
                break;
            case utils::http::METHOD::DEL:
                if (auto res = client.Delete(target, headers)) {
                    code = res->status;
                    data = res->body;
                } else {
                    throw libException(FMT("request failed ({0})",
                        httplib::to_string(res.error())));
                }
                break;
            default: throw libException("unsupported http method");
        };

        return { code, data, responseType == CONTENT_TYPE::JSON };
    };

    utils::http::METHOD method;
    string path;
    string authToken;
    Params body;
    CONTENT_TYPE contentType = CONTENT_TYPE::NON_JSON;
    CONTENT_TYPE responseType = CONTENT_TYPE::JSON;
    string serializedBody;
};
} // namespace http

namespace ws::ERROR_CODE {
const unsigned int NORMAL_CLOSURE = 1000;
const unsigned int NO_REASON = 1006;
} // namespace ws::ERROR_CODE

template <class Param>
void addParam(http::Params& bodyParams, Param& param, const string& fieldName) {
    static_assert(
        isOptional<std::decay_t<Param>>::value, "Param must be std::optional");
    if (param.has_value()) {
        string fieldValue;
        if constexpr (!std::is_same_v<typename Param::value_type, string>) {
            fieldValue = std::to_string(param.value());
        } else {
            fieldValue = param.value();
        }
        if (param.has_value()) { bodyParams.emplace(fieldName, fieldValue); }
    }
};

template <class Instrument>
inline std::vector<Instrument> parseInstruments(const std::string& data) {
    static_assert(std::is_constructible_v<Instrument, std::vector<string>>,
        "Instrument must have a constructor that accepts vector of strings");

    std::stringstream sstream(data);
    rapidcsv::Document csv(sstream, rapidcsv::LabelParams(0, -1));
    const size_t numberOfRows = csv.GetRowCount();

    std::vector<Instrument> instruments;
    for (size_t row = 0; row < numberOfRows; row++) {
        instruments.emplace_back(csv.GetRow<string>(row));
    }
    return instruments;
};

} // namespace kiteconnect::internal::utils
