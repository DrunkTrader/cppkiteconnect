# CPPKiteConnect Modernization Audit & Migration Plan

## 1. FULL REPOSITORY AUDIT

### 1.1 Current Architecture Overview

**Project Structure:**
```
/workspace/
├── CMakeLists.txt              # Main CMake config (CMake 3.10, C++17)
├── Dockerfile                   # Fedora 38 based, manual uWS build
├── README.md                    # Documentation
├── .gitmodules                  # 9 submodules
├── .github/workflows/           # CI/CD (GitHub Actions)
├── cmake/
│   ├── modules/FindGMock.cmake  # Custom GTest discovery
│   └── templates/               # Doxygen templates
├── deps/
│   └── CMakeLists.txt           # Manual dependency downloader
├── include/
│   ├── kitepp.hpp               # Main header (header-only library)
│   ├── kitepp/                  # Library headers
│   │   ├── kite.hpp             # Main kite class (729 lines)
│   │   ├── kite/*.hpp           # Module implementations (api, gtt, order, etc.)
│   │   ├── responses/*.hpp      # Response types
│   │   ├── ticker.hpp           # Ticker header
│   │   ├── ticker/*.hpp         # Ticker implementation (ws.hpp, internal.hpp)
│   │   ├── utils.hpp            # Utilities (597 lines)
│   │   └── exceptions.hpp       # Exception types
│   ├── cpp-httplib/             # Submodule: HTTP client
│   ├── fmt/                     # Submodule: Formatting
│   ├── rapidjson/               # Submodule: JSON parsing
│   ├── rapidcsv/                # Submodule: CSV parsing
│   ├── uri-parser/              # Submodule: URL parsing
│   └── PicoSHA2/                # Submodule: SHA-256 hashing
├── examples/
│   ├── example1.cpp             # Basic usage
│   ├── example2.cpp             # Comprehensive example
│   ├── example3.cpp             # GTT example
│   └── example4.cpp             # Ticker example
└── tests/
    ├── unit/                    # Unit tests
    ├── mock_custom/             # Custom mock responses
    └── mock_responses/          # Zerodha official mocks (submodule)
```

### 1.2 Current Dependencies Analysis

| Dependency | Type | Version | Usage | Location |
|------------|------|---------|-------|----------|
| **cpp-httplib** | HTTP Client | Latest (submodule) | REST API calls | `include/cpp-httplib/` |
| **uWebSockets** | WebSocket | v0.14 (manual build) | Ticker WebSocket | System-installed |
| **libuv** | Event Loop | v1.40.0 | uWS event loop | System-installed |
| **RapidJSON** | JSON Parser | Latest (submodule) | JSON serialization/deserialization | `include/rapidjson/` |
| **rapidcsv** | CSV Parser | Latest (submodule) | Instrument CSV parsing | `include/rapidcsv/` |
| **uri-parser** | URL Parser | Latest (submodule) | URL construction | `include/uri-parser/` |
| **PicoSHA2** | Hashing | Latest (submodule) | SHA-256 for session generation | `include/PicoSHA2/` |
| **fmt** | Formatting | Latest (submodule) | String formatting | `include/fmt/` |
| **GoogleTest** | Testing | v1.10.0 | Unit tests | System/submodule |
| **GoogleMock** | Mocking | v1.10.0 | Test mocking | System/submodule |
| **OpenSSL** | TLS/SSL | 1.1.1i (downloaded) | HTTPS, WSS | System/downloaded |
| **ZLIB** | Compression | 1.2.11 | WebSocket compression | System/downloaded |
| **Doxygen** | Documentation | Latest | API docs | System |

### 1.3 Code Quality Issues Identified

**Critical Architectural Flaws:**

1. **Header-Only Design**: All implementation in headers causes:
   - Slow compilation times for consumers
   - ABI instability
   - No separation of interface/implementation
   - Tight coupling with dependencies

2. **Thread Safety Violations**:
   - Ticker callbacks access shared state without synchronization
   - WebSocket operations from multiple threads without locks
   - No atomic operations for connection state

3. **Security Concerns**:
   - API secrets stored in plain `std::string`
   - No secure memory zeroing after use
   - Environment variables visible to all processes
   - Secrets potentially logged in error messages

4. **Binary Protocol Parsing**:
   - Hardcoded magic numbers (packet sizes, offsets)
   - No protocol version validation
   - No bounds checking on packet data
   - Unsafe `reinterpret_cast` usage
   - Endianness assumptions without runtime verification

5. **Error Handling**:
   - Generic "invalid body" messages for all JSON errors
   - No structured error types
   - Raw exception exposure from underlying libraries
   - No distinction between retryable/non-retryable errors

6. **Reconnect Logic**:
   - Exponential backoff starts at 0 (immediate retry)
   - Blocking sleep freezes event loop
   - No subscription restoration after reconnect
   - No state machine for connection lifecycle

7. **Rate Limiting**: Zero built-in rate limiting despite Kite API limits

8. **Input Validation**: Minimal validation of user-provided strings

### 1.4 Build System Issues

**Current CMake Problems:**
- Uses `include_directories()` instead of target-based approach
- Global compiler settings
- `file(GLOB ...)` for test file discovery
- Manual dependency finding with `find_path`/`find_library`
- No installation rules or package config generation
- No sanitizer configurations
- No CMake presets
- Minimum CMake version too old (3.10)

**CI/CD Issues:**
- Manual dependency building in CI
- No Windows support (marked TODO)
- No sanitizer testing
- No format/lint checks
- No benchmark runs
- Outdated action versions (v2)

---

## 2. DEPENDENCY MIGRATION TABLE

| Current Dependency | Current Usage | Replacement | Reason | Migration Effort | Risk | Final Status |
|-------------------|---------------|-------------|--------|------------------|------|--------------|
| **uWebSockets v0.14 + libuv** | WebSocket transport, event loop | **Boost.Beast + Boost.Asio** | v0.14 is deprecated (2018), no maintenance, poor C++ standards compliance, libuv adds complexity | High | Medium | **REPLACE** |
| **cpp-httplib** | REST HTTP client | **Boost.Beast HTTP** | Unify networking stack, better async support, maintained by Boost | Medium | Low | **REPLACE** |
| **RapidJSON** | JSON parsing/serialization | **nlohmann/json** | Modern C++ API, type-safe, better error messages, easier to use | Medium | Low | **REPLACE** |
| **rapidcsv** | Instrument CSV parsing | **Internal minimal parser** | CSV format is simple; avoids extra dependency | Low | Low | **REPLACE** |
| **uri-parser** | URL construction | **Boost.URL** | Part of Boost ecosystem, well-maintained, proper URL semantics | Low | Low | **REPLACE** |
| **PicoSHA2** | SHA-256 hashing | **OpenSSL EVP API** | Already required for TLS, modern EVP API, FIPS-compliant option | Low | Low | **REPLACE** |
| **fmt** | String formatting | **fmt (keep)** | Excellent library, C++20 compatible, used by std::format | None | None | **KEEP & UPDATE** |
| **GoogleTest/Mock** | Unit testing | **GoogleTest/Mock (keep)** | Industry standard, actively maintained | None | None | **KEEP & UPDATE** |
| **Doxygen** | Documentation | **Doxygen (keep)** | Standard for C++ API docs | None | None | **KEEP** |
| **(new)** | Benchmarking | **Google Benchmark** | Performance regression detection | Low | None | **ADD** |
| **(new)** | Fuzzing | **libFuzzer** | Security testing for binary parser | Medium | None | **ADD** |

### Final Target Dependency Stack:
```
C++20 Standard Library
├── Boost.Asio (async I/O)
├── Boost.Beast (HTTP/WebSocket)
├── Boost.URL (URL handling)
├── OpenSSL (TLS, SHA-256 via EVP)
├── nlohmann/json (JSON)
├── fmt (formatting)
├── GoogleTest (testing)
├── Google Benchmark (benchmarking)
└── libFuzzer (optional, fuzzing)
```

---

## 3. PROPOSED ARCHITECTURE

### 3.1 High-Level Architecture

```
┌─────────────────────────────────────────────────────┐
│                 Public API Layer                    │
│  (cppkiteconnect namespace, strongly typed)        │
├─────────────────────────────────────────────────────┤
│  KiteClient                                         │
│  ├── Authentication                                 │
│  ├── REST Client                                    │
│  └── Ticker Client                                  │
├─────────────────────────────────────────────────────┤
│              Internal Abstraction Layer             │
│  ├── http_transport (interface)                     │
│  ├── websocket_transport (interface)                │
│  ├── binary_decoder                                 │
│  ├── json_serializers                               │
│  └── error_handling                                 │
├─────────────────────────────────────────────────────┤
│              Implementation Layer                   │
│  ├── Boost.Beast HTTP                              │
│  ├── Boost.Beast WebSocket                         │
│  ├── Boost.Asio (event loop, timers)               │
│  └── OpenSSL (TLS, crypto)                          │
└─────────────────────────────────────────────────────┘
```

### 3.2 Directory Structure (Target)

```
cppkiteconnect/
├── CMakeLists.txt                 # Modern CMake (3.20+)
├── CMakePresets.json              # Build presets
├── vcpkg.json                     # vcpkg manifest
├── conanfile.py                   # Conan support (optional)
├── README.md
├── CHANGELOG.md
├── MIGRATION.md                   # Migration guide from v1
├── LICENSE
├── include/
│   └── cppkiteconnect/
│       ├── config.hpp             # Configuration structs
│       ├── errors.hpp             # Error types, error codes
│       ├── types.hpp              # Common types, enums
│       ├── client.hpp             # Main client facade
│       ├── authentication.hpp     # Auth logic
│       ├── rest_client.hpp        # REST transport interface
│       ├── ticker.hpp             # Ticker public API
│       ├── orders.hpp             # Order types, request structs
│       ├── portfolio.hpp          # Holdings, positions
│       ├── market_data.hpp        # Quotes, instruments, historical
│       └── gtt.hpp                # GTT types
├── src/
│   ├── client.cpp                 # Main client implementation
│   ├── http_transport.cpp         # Boost.Beast HTTP impl
│   ├── websocket_transport.cpp    # Boost.Beast WS impl
│   ├── ticker.cpp                 # Ticker logic
│   ├── binary_decoder.cpp         # Binary packet decoder
│   ├── authentication.cpp         # Session management
│   └── json.cpp                   # JSON serialization
├── tests/
│   ├── unit/
│   │   ├── test_json_parsing.cpp
│   │   ├── test_binary_decoder.cpp
│   │   ├── test_url_generation.cpp
│   │   ├── test_order_types.cpp
│   │   └── ...
│   ├── integration/
│   │   └── test_rest_api.cpp      # Requires credentials
│   ├── protocol/
│   │   └── test_ticker_protocol.cpp
│   ├── fixtures/
│   │   ├── ltp.bin
│   │   ├── quote.bin
│   │   ├── full.bin
│   │   └── malformed.bin
│   └── fuzz/
│       └── binary_decoder_fuzz.cpp
├── examples/
│   ├── authentication/
│   ├── orders/
│   ├── portfolio/
│   ├── market_data/
│   ├── ticker/
│   └── production/
├── docs/
│   └── doxygen/
├── cmake/
│   ├── modules/
│   └── templates/
└── .github/
    └── workflows/
        ├── ci.yml                 # Main CI pipeline
        ├── sanitizers.yml         # ASan, UBSan, TSan
        └── benchmarks.yml         # Performance tracking
```

### 3.3 Class Design (Key Components)

#### 3.3.1 Error Handling

```cpp
namespace cppkiteconnect {

enum class error_code {
    success = 0,
    network_failure,
    timeout,
    tls_failure,
    authentication_failed,
    authorization_failed,
    rate_limited,
    invalid_request,
    api_error,
    websocket_failure,
    protocol_error,
    parse_error,
    instrument_not_found,
    order_rejected
};

struct error_info {
    error_code code;
    int http_status{0};
    std::string message;
    std::string api_error_type;  // Kite-specific error type
    std::string request_id;       // For debugging
};

class kite_exception : public std::runtime_error {
public:
    explicit kite_exception(error_info info);
    [[nodiscard]] error_info const& info() const noexcept;
    [[nodiscard]] error_code code() const noexcept;
    [[nodiscard]] int http_status() const noexcept;
private:
    error_info info_;
};

// Specific exception types
class authentication_error : public kite_exception { /*...*/ };
class rate_limit_error : public kite_exception {
    std::chrono::seconds retry_after;
};
class network_error : public kite_exception { /*...*/ };

} // namespace cppkiteconnect
```

#### 3.3.2 Configuration

```cpp
namespace cppkiteconnect {

struct retry_policy {
    std::size_t max_attempts{3};
    std::chrono::milliseconds initial_delay{100};
    std::chrono::milliseconds max_delay{5000};
    double backoff_multiplier{2.0};

    // Non-retryable error codes
    std::set<error_code> non_retryable{
        error_code::authentication_failed,
        error_code::authorization_failed,
        error_code::invalid_request
    };
};

struct rate_limit_policy {
    std::size_t requests_per_second{3};
    std::size_t burst_size{5};
};

struct client_config {
    // Credentials
    std::string api_key;
    std::string access_token;  // Set after authentication

    // Endpoints (configurable for sandbox/testing)
    std::string rest_endpoint{"https://api.kite.trade"};
    std::string websocket_endpoint{"wss://websocket.kite.trade"};

    // Timeouts
    std::chrono::milliseconds connection_timeout{7000};
    std::chrono::milliseconds request_timeout{7000};

    // Policies
    retry_policy retry;
    rate_limit_policy rate_limit;

    // Optional: custom root CA for TLS
    std::string ca_cert_path;

    // Logging level
    enum class log_level { trace, debug, info, warning, error, none };
    log_level logging{log_level::warning};
};

} // namespace cppkiteconnect
```

#### 3.3.3 Strongly Typed Enums

```cpp
namespace cppkiteconnect {

enum class exchange {
    nse,
    bse,
    nfo,
    cde,
    bfo,
    mcx
};

enum class transaction_type {
    buy,
    sell
};

enum class product {
    CNC,
    NRML,
    MIS,
    CO,
    BO
};

enum class order_type {
    MARKET,
    LIMIT,
    SL,
    SLM
};

enum class validity {
    DAY,
    IOC,    // Immediate or cancel
    TTL     // Time to live (for BO/CO)
};

enum class variety {
    REGULAR,
    BO,     // Bracket order
    CO,     // Cover order
    AMO     // After market order
};

enum class position_type {
    net,
    day
};

enum class subscription_mode {
    quote,
    ltp,
    full
};

enum class connection_state {
    disconnected,
    connecting,
    connected,
    reconnecting,
    stopping
};

// Conversion utilities
[[nodiscard]] std::string to_string(exchange e);
[[nodiscard]] exchange exchange_from_string(std::string_view s);
// ... similar for other enums

} // namespace cppkiteconnect
```

#### 3.3.4 Request Structures (Modern API)

```cpp
namespace cppkiteconnect {

struct order_request {
    exchange exchange;
    std::string tradingsymbol;
    transaction_type transaction_type;
    product product;
    order_type order_type;
    validity validity{validity::DAY};

    int32_t quantity{};
    int32_t disclosed_quantity{};  // optional

    std::optional<double> price;
    std::optional<double> trigger_price;

    std::optional<int32_t> squareoff;
    std::optional<double> stoploss;
    std::optional<double> trailing_stoploss;

    std::optional<std::string> tag;  // Order tag
};

struct modify_order_request {
    std::string order_id;
    variety variety{variety::REGULAR};

    std::optional<int32_t> quantity;
    std::optional<double> price;
    std::optional<order_type> order_type;
    std::optional<validity> validity;
    std::optional<double> trigger_price;
    std::optional<int32_t> disclosed_quantity;
};

struct convert_position_request {
    exchange exchange;
    std::string tradingsymbol;
    position_type position_type;
    product old_product;
    product new_product;
    transaction_type transaction_type;
    int32_t quantity{};
};

} // namespace cppkiteconnect
```

#### 3.3.5 Ticker Callback API

```cpp
namespace cppkiteconnect {

class ticker {
public:
    using tick_callback = std::function<void(std::span<const tick> ticks)>;
    using connect_callback = std::function<void()>;
    using close_callback = std::function<void(int code, std::string_view reason)>;
    using error_callback = std::function<void(error_info const& error)>;
    using reconnect_callback = std::function<void(int attempt)>;

    explicit ticker(client_config const& config);
    ~ticker();

    // Connection management
    void connect();
    void disconnect();

    // Subscription management
    void subscribe(std::vector<int32_t> instrument_tokens);
    void unsubscribe(std::vector<int32_t> instrument_tokens);
    void set_mode(subscription_mode mode, std::vector<int32_t> instrument_tokens);

    // Callback registration
    void on_connect(connect_callback cb);
    void on_ticks(tick_callback cb);
    void on_close(close_callback cb);
    void on_error(error_callback cb);
    void on_reconnecting(reconnect_callback cb);
    void on_reconnect_fail(reconnect_callback cb);

    // Thread safety note: All callbacks execute on the ticker's internal thread
    // Do not call ticker methods from within callbacks to avoid deadlocks

private:
    struct impl;
    std::unique_ptr<impl> pimpl_;  // Hide implementation details
};

} // namespace cppkiteconnect
```

---

## 4. PUBLIC API MIGRATION STRATEGY

### 4.1 Breaking Changes Summary

| Old API | New API | Rationale |
|---------|---------|-----------|
| `kite(string apikey)` | `kite_client(client_config config)` | Centralized configuration |
| `placeOrder(placeOrderParams)` | `place_order(order_request)` | Consistent naming, strongly typed |
| `cancelOrder(variety, orderId, parentOrderId)` | `cancel_order(cancel_request)` | Structured parameters |
| `int placeGtt(...)` | `int64_t place_gtt(gtt_request)` | Prevent overflow, consistent naming |
| `string getAccessToken()` | Removed (internal state) | Security: don't expose tokens |
| Public callback members (`onConnect`, etc.) | `on_connect(callback)` methods | Better encapsulation |
| `std::vector<order> orders()` | `get_orders()` | Consistent verb-noun pattern |
| Raw string enums (`"NSE"`, `"BUY"`) | `exchange::nse`, `transaction_type::buy` | Type safety |
| `-1`, `""`, `0` for optional values | `std::optional<T>` | Clear intent, no magic values |
| `int timeout` parameters | `std::chrono::milliseconds` | Type-safe durations |

### 4.2 Backward Compatibility Approach

**Decision**: Introduce clean v2 API with deprecation warnings for v1

**Rationale**:
- Maintaining full backward compatibility would compromise design goals
- Header-only to compiled library transition is breaking by nature
- Clean break allows proper error handling, threading model changes
- Users can migrate incrementally with clear guidance

**Deprecation Strategy**:
```cpp
namespace kiteconnect {
// Old API remains but marked deprecated
[[deprecated("Use cppkiteconnect::kite_client instead")]]
class kite { /* ... */ };
}

// Users get compile-time warnings pointing to migration guide
```

### 4.3 Migration Guide Structure (MIGRATION.md)

The migration document will include:

1. **Quick Start**: Side-by-side comparison of common operations
2. **Dependency Changes**: How to install new dependencies
3. **Build System Changes**: CMake integration examples
4. **API Mapping Table**: Every old function mapped to new equivalent
5. **Error Handling Migration**: Exception-based to error_code pattern
6. **Threading Model Changes**: Callback semantics explanation
7. **Common Pitfalls**: Gotchas during migration
8. **Full Example Rewrite**: Complete example1.cpp rewritten in v2 style

---

## 5. BUILD SYSTEM MIGRATION STRATEGY

### 5.1 CMake Modernization Plan

**Current Issues to Address**:
- Replace `include_directories()` with target-based approach
- Remove `file(GLOB ...)` usage
- Add proper installation rules
- Generate package config files
- Support vcpkg manifest mode
- Add CMake presets for common configurations
- Minimum version bump to 3.20+

**Target CMakeLists.txt Structure**:

```cmake
cmake_minimum_required(VERSION 3.20)
project(cppkiteconnect
    VERSION 2.0.0
    DESCRIPTION "Modern C++ Kite Connect SDK"
    LANGUAGES CXX
)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(CPPKITECONNECT_BUILD_TESTS "Build unit tests" OFF)
option(CPPKITECONNECT_BUILD_EXAMPLES "Build examples" OFF)
option(CPPKITECONNECT_BUILD_DOCS "Build documentation" OFF)
option(CPPKITECONNECT_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(CPPKITECONNECT_SANITIZE_ADDRESS "Enable ASan" OFF)
option(CPPKITECONNECT_SANITIZE_UB "Enable UBSan" OFF)
option(CPPKITECONNECT_SANITIZE_THREAD "Enable TSan" OFF)

# Find dependencies (via vcpkg/package manager)
find_package(Boost REQUIRED COMPONENTS beast url)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)

if(CPPKITECONNECT_BUILD_TESTS)
    find_package(GTest REQUIRED)
endif()

if(CPPKITECONNECT_BUILD_BENCHMARKS)
    find_package(benchmark REQUIRED)
endif()

# Library target
add_library(cppkiteconnect
    src/client.cpp
    src/http_transport.cpp
    src/websocket_transport.cpp
    src/ticker.cpp
    src/binary_decoder.cpp
    src/authentication.cpp
    src/json.cpp
)

target_include_directories(cppkiteconnect
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(cppkiteconnect
    PUBLIC
        Boost::beast
        Boost::url
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        fmt::fmt
    PRIVATE
        Threads::Threads
)

target_compile_features(cppkiteconnect PUBLIC cxx_std_20)

# Sanitizer support
if(CPPKITECONNECT_SANITIZE_ADDRESS)
    target_compile_options(cppkiteconnect PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(cppkiteconnect PRIVATE -fsanitize=address)
endif()

# Installation rules
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS cppkiteconnect
    EXPORT cppkiteconnectTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/cppkiteconnect
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Package config generation
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cppkiteconnectConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cppkiteconnect
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/cppkiteconnectConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT cppkiteconnectTargets
    FILE cppkiteconnectTargets.cmake
    NAMESPACE CPPKiteConnect::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cppkiteconnect
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/cppkiteconnectConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cppkiteconnectConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cppkiteconnect
)
```

### 5.2 vcpkg Manifest (vcpkg.json)

```json
{
  "name": "cppkiteconnect",
  "version": "2.0.0",
  "description": "Modern C++ Kite Connect SDK",
  "homepage": "https://github.com/zerodha/cppkiteconnect",
  "license": "MIT",
  "supports": "(windows & x64) | (linux & x64) | (osx & x64)",

  "dependencies": [
    {
      "name": "boost-beast",
      "version>=": "1.83.0"
    },
    {
      "name": "boost-url",
      "version>=": "1.83.0"
    },
    {
      "name": "openssl",
      "version>=": "3.0.0"
    },
    {
      "name": "nlohmann-json",
      "version>=": "3.11.0"
    },
    {
      "name": "fmt",
      "version>=": "10.0.0"
    }
  ],

  "builtin-baseline": "2024.01.12",

  "features": {
    "tests": {
      "description": "Build unit tests",
      "dependencies": [
        {
          "name": "gtest",
          "version>=": "1.14.0"
        }
      ]
    },
    "benchmarks": {
      "description": "Build benchmarks",
      "dependencies": [
        {
          "name": "benchmark",
          "version>=": "1.8.0"
        }
      ]
    }
  }
}
```

### 5.3 CMakePresets.json

```json
{
  "version": 6,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 20,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "base",
      "hidden": true,
      "generator": "Ninja Multi-Config",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "CMAKE_CXX_STANDARD": "20"
      }
    },
    {
      "name": "debug",
      "inherits": "base",
      "displayName": "Debug Build",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CPPKITECONNECT_BUILD_TESTS": "ON",
        "CPPKITECONNECT_BUILD_EXAMPLES": "ON"
      }
    },
    {
      "name": "release",
      "inherits": "base",
      "displayName": "Release Build",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release"
      }
    },
    {
      "name": "asan",
      "inherits": "debug",
      "displayName": "AddressSanitizer Build",
      "cacheVariables": {
        "CPPKITECONNECT_SANITIZE_ADDRESS": "ON",
        "CPPKITECONNECT_SANITIZE_UB": "ON"
      }
    },
    {
      "name": "ubsan",
      "inherits": "debug",
      "displayName": "UndefinedBehaviorSanitizer Build",
      "cacheVariables": {
        "CPPKITECONNECT_SANITIZE_UB": "ON"
      }
    },
    {
      "name": "tsan",
      "inherits": "debug",
      "displayName": "ThreadSanitizer Build",
      "cacheVariables": {
        "CPPKITECONNECT_SANITIZE_THREAD": "ON"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "debug",
      "configurePreset": "debug"
    },
    {
      "name": "release",
      "configurePreset": "release"
    },
    {
      "name": "asan",
      "configurePreset": "asan"
    }
  ],
  "testPresets": [
    {
      "name": "debug",
      "configurePreset": "debug",
      "output": {
        "outputOnFailure": true
      }
    },
    {
      "name": "asan",
      "configurePreset": "asan",
      "output": {
        "outputOnFailure": true
      }
    }
  ]
}
```

---

## 6. RISK ASSESSMENT

### 6.1 Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| **Boost.Beast API changes** | Low | Medium | Pin Boost version, wrap transport layer |
| **OpenSSL EVP complexity** | Low | Low | Create simple wrapper, test thoroughly |
| **Binary decoder bugs** | Medium | High | Extensive fuzzing, fixture-based testing |
| **Thread safety issues** | Medium | High | Use std::jthread, mutexes, minimal shared state |
| **Performance regression** | Low | Medium | Benchmark suite, profile before/after |
| **Windows compatibility** | Medium | Medium | Test on all three platforms in CI |
| **vcpkg package availability** | Low | Low | Fallback to system packages if needed |

### 6.2 Migration Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| **Breaking existing users** | High | Medium | Deprecation warnings, migration guide, v1 branch maintenance |
| **Learning curve for new API** | Medium | Low | Comprehensive examples, documentation |
| **Dependency installation issues** | Medium | Low | Multiple package manager support (vcpkg, Conan, system) |
| **Incomplete test coverage** | Medium | High | Prioritize critical path tests, add fuzzing |

### 6.3 Timeline Estimate

| Phase | Duration | Dependencies |
|-------|----------|--------------|
| Phase 1: Audit & Planning | 1 week | None |
| Phase 2: CMake & Dependencies | 1 week | Phase 1 |
| Phase 3: C++20 Core Migration | 2 weeks | Phase 2 |
| Phase 4: HTTP Transport | 2 weeks | Phase 3 |
| Phase 5: JSON/Parser Migration | 1 week | Phase 3 |
| Phase 6: URL/Hash/CSV | 1 week | Phase 3 |
| Phase 7: WebSocket/Ticker | 3 weeks | Phase 4, 5 |
| Phase 8: Public API | 2 weeks | Phase 4-7 |
| Phase 9: Testing/Sanitizers/Fuzzing | 2 weeks | Phase 8 |
| Phase 10: Packaging/Docs/CI | 2 weeks | Phase 9 |
| **Total** | **16 weeks (~4 months)** | |

---

## 7. ORDERED IMPLEMENTATION PLAN

### PHASE 1: Audit & Planning (COMPLETED)
- [x] Repository audit
- [x] Dependency analysis
- [x] Architecture design
- [x] Migration plan creation
- [ ] Risk assessment review
- [ ] Stakeholder approval

### PHASE 2: Modern CMake & Dependency Management
**Goal**: Establish build foundation with modern CMake and package management

**Tasks**:
1. Create new CMakeLists.txt with target-based approach
2. Write vcpkg.json manifest
3. Create CMakePresets.json
4. Set up cmake/modules for any custom finds
5. Remove old submodule references from .gitmodules
6. Test basic configuration with vcpkg

**Deliverables**:
- Working CMake build with no source code
- vcpkg manifest that resolves all dependencies
- CMake presets for debug/release/sanitizers

**Success Criteria**:
```bash
cmake --preset=debug
cmake --build --preset=debug
# Builds successfully with all dependencies found
```

### PHASE 3: C++20 Core Migration
**Goal**: Establish core types, enums, and error handling

**Tasks**:
1. Create include/cppkiteconnect/types.hpp with enums
2. Create include/cppkiteconnect/errors.hpp with error types
3. Create include/cppkiteconnect/config.hpp with configuration structs
4. Implement enum string conversions
5. Set up logging abstraction (optional)
6. Create src/json.cpp for nlohmann/json utilities

**Deliverables**:
- Strongly typed enums (exchange, transaction_type, etc.)
- Error hierarchy with exception classes
- Configuration structs with chrono durations
- Basic JSON serialization helpers

**Success Criteria**:
- All enums convertible to/from strings
- Error types compile and can be thrown/caught
- Configuration struct usable in client code

### PHASE 4: HTTP Transport Migration
**Goal**: Replace cpp-httplib with Boost.Beast HTTP

**Tasks**:
1. Create internal http_transport interface
2. Implement Boost.Beast HTTP client
3. Support GET, POST, PUT, DELETE
4. Implement connection pooling
5. Add timeout handling
6. Implement retry logic with backoff
7. Add rate limiting (token bucket)
8. Map HTTP errors to sdk error codes

**Deliverables**:
- src/http_transport.cpp
- include/cppkiteconnect/internal/http_transport.hpp
- Unit tests for HTTP operations

**Success Criteria**:
- Can make HTTPS requests to test endpoints
- Timeouts work correctly
- Retry logic doesn't retry non-retryable errors
- Rate limiting prevents >3 req/sec

### PHASE 5: JSON/Parser Migration
**Goal**: Replace RapidJSON with nlohmann/json

**Tasks**:
1. Replace all RapidJSON includes with nlohmann/json
2. Convert JSON parsing to use from_json/to_json
3. Improve error messages with field names
4. Create response type serializers
5. Handle optional fields properly
6. Test all response types against mocks

**Deliverables**:
- Updated response types in include/cppkiteconnect/
- JSON serializers in src/json.cpp
- Unit tests for JSON parsing

**Success Criteria**:
- All mock responses parse correctly
- Missing fields produce clear error messages
- Optional fields handled with std::optional

### PHASE 6: URL/Hash/CSV Migration
**Goal**: Replace uri-parser, PicoSHA2, rapidcsv

**Tasks**:
1. Replace uri-parser with Boost.URL
2. Replace PicoSHA2 with OpenSSL EVP
3. Evaluate CSV parser need vs internal implementation
4. Implement minimal CSV parser if needed
5. Update login URL generation
6. Update session generation with SHA-256

**Deliverables**:
- URL construction using Boost.URL
- SHA-256 hashing with OpenSSL EVP
- CSV parsing (internal or csv-parser)

**Success Criteria**:
- Login URL matches expected format
- Session generation works with test credentials
- Instrument CSV parses correctly

### PHASE 7: WebSocket/Ticker Migration
**Goal**: Replace uWebSockets with Boost.Beast WebSocket

**Tasks**:
1. Create websocket_transport interface
2. Implement Boost.Beast WebSocket client
3. Implement async connect/disconnect
4. Implement reconnection logic with backoff
5. Implement subscription tracking
6. Implement binary packet decoder
7. Add thread-safe callback dispatch
8. Implement clean shutdown

**Deliverables**:
- src/websocket_transport.cpp
- src/ticker.cpp
- src/binary_decoder.cpp
- include/cppkiteconnect/ticker.hpp
- Unit tests with binary fixtures

**Success Criteria**:
- Can connect to WebSocket server
- Reconnects automatically on failure
- Subscriptions restored after reconnect
- Binary packets decode correctly
- No data races (TSan clean)

### PHASE 8: Public API Implementation
**Goal**: Implement main client facade with modern API

**Tasks**:
1. Create include/cppkiteconnect/client.hpp
2. Implement authentication methods
3. Implement order methods
4. Implement portfolio methods
5. Implement market data methods
6. Implement GTT methods
7. Implement MF methods
8. Add [[nodiscard]] attributes
9. Add deprecation wrappers for v1 API

**Deliverables**:
- Complete client API
- Request/response structs
- Deprecation layer for v1

**Success Criteria**:
- All Kite Connect endpoints covered
- API follows naming conventions
- Examples compile and run

### PHASE 9: Testing, Sanitizers, Fuzzing, Benchmarks
**Goal**: Ensure quality, performance, and security

**Tasks**:
1. Write unit tests for all components
2. Create binary packet fixtures
3. Set up libFuzzer for binary decoder
4. Write Google Benchmark benchmarks
5. Run ASan, UBSan, TSan tests
6. Achieve >80% code coverage
7. Fix all sanitizer warnings

**Deliverables**:
- tests/unit/ with comprehensive tests
- tests/fixtures/ with binary data
- tests/fuzz/ with fuzz targets
- benchmarks/ with performance tests
- CI pipeline with sanitizer runs

**Success Criteria**:
- All tests pass on Linux/macOS/Windows
- No sanitizer errors
- Fuzzing finds no crashes
- Benchmarks show no regression

### PHASE 10: Packaging, Documentation, CI/CD
**Goal**: Production-ready release

**Tasks**:
1. Write README with installation instructions
2. Write MIGRATION.md guide
3. Write CHANGELOG.md
4. Update Doxygen configuration
5. Create GitHub Actions workflow
6. Add format/lint checks
7. Add Windows CI jobs
8. Test installation from package
9. Create release package

**Deliverables**:
- Complete documentation
- Working CI/CD pipeline
- Installable package
- Release artifacts

**Success Criteria**:
- `find_package(CPPKiteConnect)` works
- Documentation builds successfully
- CI passes on all platforms
- Examples run successfully

---

## 8. NEXT STEPS

This migration plan provides a comprehensive roadmap for modernizing cppkiteconnect. The next steps are:

1. **Review and approve this plan** - Ensure all stakeholders agree with the approach
2. **Set up development environment** - Install Boost, vcpkg, required tools
3. **Begin Phase 2** - Start with CMake and dependency management
4. **Iterate through phases** - Complete each phase before moving to the next
5. **Maintain v1 branch** - Keep existing code working during migration
6. **Release v2 beta** - Get early feedback from users
7. **Final release** - After thorough testing and documentation

**Key Success Factors**:
- Don't skip testing phases
- Measure performance before optimizing
- Document breaking changes clearly
- Maintain backward compatibility where practical
- Test on all target platforms continuously