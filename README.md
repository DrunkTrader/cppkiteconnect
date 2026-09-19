# Kite Connect API C++ Client (Modern C++20)

[![CI](https://github.com/zerodha/cppkiteconnect/actions/workflows/ci.yml/badge.svg)](https://github.com/zerodha/cppkiteconnect/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Complete C++20 migration with modern architecture, Boost.Beast HTTP/WebSocket, nlohmann/json, and comprehensive testing.**

- [Overview](#overview)
- [What's New in C++20 Migration](#whats-new-in-c20-migration)
- [Dependencies](#dependencies)
- [Quick Start](#quick-start)
  - [Using vcpkg (Recommended)](#using-vcpkg-recommended)
  - [Using System Packages](#using-system-packages)
- [Building & Installation](#building--installation)
  - [Build Presets](#build-presets)
  - [Build Options](#build-options)
- [Examples](#examples)
  - [REST API Example](#rest-api-example)
  - [WebSocket Ticker Example](#websocket-ticker-example)
- [Architecture](#architecture)
- [Documentation](#documentation)
- [Testing](#testing)
- [Migration Status](#migration-status)
- [License](#license)

## Overview

The official C++ client for communicating with [Kite Connect API](https://kite.trade/).

**This is a complete rewrite using modern C++20 features**, replacing legacy dependencies with Boost.Beast (HTTP/WebSocket), nlohmann/json, and OpenSSL. The library provides type-safe, coroutine-ready interfaces for both REST and WebSocket APIs with comprehensive error handling and modern C++ idioms.

### Key Features

- ✅ **C++20 Standard** - Modern language features (concepts, ranges, coroutines-ready)
- ✅ **Type-Safe Enums** - Strongly typed enum classes with string conversion
- ✅ **Modern Error Handling** - Exception hierarchy with retry policies
- ✅ **Boost.Beast HTTP** - Async HTTP client with SSL/TLS support
- ✅ **Boost.Beast WebSocket** - Real-time ticker with auto-reconnect
- ✅ **nlohmann/json** - Modern JSON parsing with optional field support
- ✅ **Comprehensive Testing** - Unit tests, sanitizers, fuzzing, benchmarks
- ✅ **Cross-Platform** - Linux, macOS, Windows support via vcpkg
- ✅ **CMake Presets** - Pre-configured build configurations
- ✅ **CI/CD Pipeline** - Automated testing with multiple configurations

## What's New in C++20 Migration

This project has undergone a complete 10-phase migration from legacy C++17 code to modern C++20:

### Phase 1: Planning & Risk Assessment ✅
- Comprehensive risk assessment and mitigation strategies
- Repository audit and dependency analysis
- GitHub infrastructure improvements (templates, workflows)

### Phase 2: Build System Modernization ✅
- **CMake 3.20+** with target-based dependency management
- **vcpkg manifest** for reproducible builds
- **CMakePresets.json** for standardized configurations
- Sanitizer support (ASan, UBSan, TSan)

### Phase 3: C++20 Core Types ✅
- Strongly typed enums (exchange, order_type, product, etc.)
- Modern exception hierarchy with error_info
- Request structures with fluent builders
- Configuration structs with chrono durations

### Phase 4: HTTP Transport (Boost.Beast) ✅
- Abstract HTTP client interface
- Boost.Beast implementation with coroutines
- Mock client for unit testing
- SSL/TLS support with configurable timeouts

### Phase 5: JSON Migration (nlohmann/json) ✅
- Complete replacement of RapidJSON
- Type-safe deserialization with `from_json`
- Optional field support with `std::optional`
- Better error messages and diagnostics

### Phase 6: URL/Hash/CSV Utilities ✅
- Custom URL builder (replaces uri-parser)
- SHA-256 hashing via OpenSSL (replaces PicoSHA2)
- RFC 4180 compliant CSV parser (replaces rapidcsv)

### Phase 7: WebSocket Migration (Boost.Beast) ✅
- Boost.Beast WebSocket client (replaces uWebSockets)
- Binary packet decoder for LTP/Quote/Full modes
- Auto-reconnect with exponential backoff
- Thread-safe callbacks

### Phase 8: Public API Implementation ✅
- Modern `kite_client` facade with pimpl pattern
- Complete REST API coverage (orders, portfolio, market data, GTT, MF)
- WebSocket ticker integration
- Response parsers for all endpoint types

### Phase 9: Testing & QA Infrastructure ✅
- Comprehensive unit tests (GoogleTest)
- Sanitizer integration (ASan, UBSan, TSan)
- LibFuzzer for fuzz testing
- Google Benchmarks for performance
- Code coverage reporting (gcov/lcov)

### Phase 10: Documentation & Examples ✅
- Doxygen API reference with modern theme
- User guide and migration documentation
- 6 standalone examples covering all features
- CI/CD integration for docs deployment

## Dependencies

### Required

- **C++20** compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- **CMake 3.20+**
- **OpenSSL** (SSL/TLS, hashing)
- **ZLIB** (compression)
- **Boost** (Asio, Beast, URL) - via vcpkg or system packages
- **nlohmann/json** - via vcpkg or system packages

### Optional (for development)

- **vcpkg** (recommended package manager)
- **GoogleTest** (unit testing)
- **Doxygen** (documentation generation)
- **clang-format** (code formatting)

## Quick Start

### Using vcpkg (Recommended)

The easiest way to build is using vcpkg with manifest mode:

```bash
# Clone the repository
git clone https://github.com/zerodha/cppkiteconnect.git
cd cppkiteconnect

# Configure with vcpkg (automatic dependency installation)
cmake --preset=debug

# Build
cmake --build --preset=debug

# Run tests
ctest --preset=debug
```

vcpkg will automatically install all dependencies:
- boost-asio, boost-beast, boost-url
- nlohmann-json
- openssl, zlib

### Using System Packages

For Linux distributions with packaged dependencies:

```bash
# Ubuntu/Debian
sudo apt install libboost-all-dev libssl-dev zlib1g-dev nlohmann-json3-dev

# Fedora
sudo dnf install boost-devel openssl-devel zlib-devel nlohmann-json-devel

# Configure without vcpkg
cmake --preset=debug-novcpkg

# Build
cmake --build --preset=debug
```

## Building & Installation

### Build Presets

The project includes pre-configured CMake presets:

| Preset | Description | Use Case |
|--------|-------------|----------|
| `debug` | Debug build with symbols | Development, debugging |
| `release` | Optimized release build | Production deployment |
| `relwithdebinfo` | Release with debug info | Profiling, release debugging |
| `asan` | AddressSanitizer build | Memory error detection |
| `ubsan` | UndefinedBehaviorSanitizer | UB detection |
| `tsan` | ThreadSanitizer build | Race condition detection |
| `debug-novcpkg` | Debug without vcpkg | System package builds |

```bash
# List available presets
cmake --list-presets

# Configure a specific preset
cmake --preset=debug

# Build
cmake --build --preset=debug

# Run tests
ctest --preset=debug

# Install (optional)
cmake --install --preset=debug --prefix /usr/local
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `CPPKITE_BUILD_TESTS` | OFF | Build unit tests |
| `CPPKITE_BUILD_EXAMPLES` | OFF | Build example programs |
| `CPPKITE_BUILD_DOCS` | OFF | Build Doxygen documentation |
| `CPPKITE_BUILD_BENCHMARKS` | OFF | Build Google Benchmarks |
| `CPPKITE_ENABLE_SANITIZERS` | OFF | Enable sanitizer instrumentation |
| `CPPKITE_ENABLE_COVERAGE` | OFF | Enable code coverage |
| `CPPKITE_USE_VCPKG` | ON | Use vcpkg for dependencies |

Example with custom options:

```bash
cmake --preset=debug \
  -DCPPKITE_BUILD_TESTS=ON \
  -DCPPKITE_BUILD_EXAMPLES=ON \
  -DCPPKITE_BUILD_DOCS=ON
```

## Examples

Six comprehensive examples are provided in the `examples/` directory:

### REST API Example

```cpp
#include <iostream>
#include <cppkiteconnect/client.hpp>
#include <cppkiteconnect/errors.hpp>

namespace kc = cppkiteconnect;

int main() {
    try {
        // Initialize client with API key
        kc::kite_client client("your_api_key");
        
        // Get login URL and authenticate manually
        std::string login_url = client.get_login_url();
        std::cout << "Login URL: " << login_url << std::endl;
        std::cout << "Authenticate in browser and enter request token: ";
        
        std::string request_token;
        std::cin >> request_token;
        
        // Generate session
        std::string api_secret = "your_api_secret";
        auto session = client.generate_session(request_token, api_secret);
        std::cout << "Access token: " << session.access_token << std::endl;
        
        // Get user profile
        auto profile = client.get_profile();
        std::cout << "Welcome, " << profile.user_name << "!" << std::endl;
        std::cout << "Email: " << profile.email << std::endl;
        
        // Get orders
        auto orders = client.get_orders();
        std::cout << "Total orders: " << orders.size() << std::endl;
        
    } catch (const kc::authentication_error& e) {
        std::cerr << "Authentication failed: " << e.what() << std::endl;
    } catch (const kc::network_error& e) {
        std::cerr << "Network error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}
```

### WebSocket Ticker Example

```cpp
#include <iostream>
#include <cppkiteconnect/ticker.hpp>
#include <cppkiteconnect/types.hpp>

namespace kc = cppkiteconnect;

void on_connect(kc::ticker& ws) {
    std::cout << "Connected to WebSocket!" << std::endl;
    
    // Subscribe to instruments
    ws.subscribe({408065, 2953217});
    
    // Set mode to full
    ws.set_mode(kc::subscription_mode::full, {408065, 2953217});
}

void on_ticks(kc::ticker& ws, const std::vector<kc::tick>& ticks) {
    for (const auto& tick : ticks) {
        std::cout << "Instrument: " << tick.instrument_token
                  << " | LTP: " << tick.last_price
                  << " | Volume: " << tick.volume
                  << std::endl;
    }
}

void on_error(kc::ticker& ws, int code, const std::string& message) {
    std::cerr << "WebSocket error " << code << ": " << message << std::endl;
}

void on_close(kc::ticker& ws, int code, const std::string& reason) {
    std::cout << "WebSocket closed: " << reason << std::endl;
}

int main() {
    try {
        // Initialize ticker
        kc::ticker ws("your_api_key", "your_access_token");
        
        // Set callbacks
        ws.on_connect = on_connect;
        ws.on_ticks = on_ticks;
        ws.on_error = on_error;
        ws.on_close = on_close;
        
        // Connect and run
        ws.connect();
        ws.run();  // Blocking call
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

### Running Examples

```bash
# Build examples
cmake --preset=debug -DCPPKITE_BUILD_EXAMPLES=ON
cmake --build --preset=debug

# Run example 1 (basic REST API)
./build/debug/examples/example1

# Or use environment variables
export KITE_API_KEY="your_key"
export KITE_ACCESS_TOKEN="your_token"
export KITE_API_SECRET="your_secret"
./build/debug/examples/example1
```

## Architecture

The library follows a modern layered architecture:

```
┌─────────────────────────────────────┐
│         Public API (kite_client)    │  ← Facade pattern, pimpl idiom
├─────────────────────────────────────┤
│    HTTP Client (Boost.Beast)        │  ← Async, coroutine-ready
│    WebSocket Client (Boost.Beast)   │  ← Auto-reconnect, binary decoder
├─────────────────────────────────────┤
│       JSON Parser (nlohmann/json)   │  ← Type-safe deserialization
│       URL Builder                   │  ← Fluent interface
│       CSV Parser                    │  ← RFC 4180 compliant
│       Crypto (OpenSSL SHA-256)      │  ← Session hashing
├─────────────────────────────────────┤
│         C++20 Core Types            │  ← Enum classes, config structs
│         Error Handling              │  ← Exception hierarchy
└─────────────────────────────────────┘
```

### Key Design Principles

1. **Modern C++20**: Leverages latest language features
2. **Type Safety**: Strongly typed enums and structs
3. **Error Handling**: Comprehensive exception hierarchy
4. **Separation of Concerns**: Clean layer separation
5. **Testability**: Mock interfaces for unit testing
6. **Performance**: Zero-copy where possible, move semantics
7. **Thread Safety**: Thread-safe callbacks and state management

## Documentation

### API Reference

Generate Doxygen documentation:

```bash
cmake --preset=debug -DCPPKITE_BUILD_DOCS=ON
cmake --build --preset=debug --target docs
```

Open `build/debug/docs/html/index.html` in your browser.

### User Guide

See `docs/` directory for comprehensive guides:
- `mainpage.md` - Main documentation page
- `PHASE*_COMPLETION_REPORT.md` - Detailed phase reports
- `RISK_ASSESSMENT.md` - Risk analysis and mitigation

### Kite Connect API Documentation

- [REST API Reference](https://kite.trade/docs/connect/v3/)
- [WebSocket API Reference](https://kite.trade/docs/connect/v3/websockets/)

## Testing

### Running Tests

```bash
# Build with tests
cmake --preset=debug -DCPPKITE_BUILD_TESTS=ON
cmake --build --preset=debug

# Run all tests
ctest --preset=debug

# Run with verbose output
ctest --preset=debug --verbose

# Run specific test
ctest --preset=debug -R test_types
```

### Sanitizers

```bash
# AddressSanitizer (memory errors)
cmake --preset=asan -DCPPKITE_BUILD_TESTS=ON
cmake --build --preset=asan
ctest --preset=asan

# UndefinedBehaviorSanitizer
cmake --preset=ubsan -DCPPKITE_BUILD_TESTS=ON
cmake --build --preset=ubsan
ctest --preset=ubsan

# ThreadSanitizer (race conditions)
cmake --preset=tsan -DCPPKITE_BUILD_TESTS=ON
cmake --build --preset=tsan
ctest --preset=tsan
```

### Code Coverage

```bash
cmake --preset=debug \
  -DCPPKITE_BUILD_TESTS=ON \
  -DCPPKITE_ENABLE_COVERAGE=ON
cmake --build --preset=debug
ctest --preset=debug

# Generate coverage report
cmake --build --preset=debug --target coverage
```

### Fuzz Testing

```bash
cmake --preset=debug -DCPPKITE_BUILD_FUZZERS=ON
cmake --build --preset=debug

# Run CSV parser fuzzer
./build/debug/fuzzers/csv_parser_fuzzer fuzz_corpus/
```

### Benchmarks

```bash
cmake --preset=release -DCPPKITE_BUILD_BENCHMARKS=ON
cmake --build --preset=release

# Run benchmarks
./build/release/benchmarks/json_benchmark
./build/release/benchmarks/csv_benchmark
```

## Migration Status

All 10 phases of the C++20 migration are **COMPLETE**:

| Phase | Status | Description |
|-------|--------|-------------|
| 1 | ✅ | Planning, Risk Assessment, GitHub Infrastructure |
| 2 | ✅ | Build System (CMake, vcpkg, Presets) |
| 3 | ✅ | C++20 Core Types (Enums, Errors, Requests) |
| 4 | ✅ | HTTP Transport (Boost.Beast) |
| 5 | ✅ | JSON Parser (nlohmann/json) |
| 6 | ✅ | URL/Hash/CSV Utilities |
| 7 | ✅ | WebSocket (Boost.Beast) |
| 8 | ✅ | Public API Implementation |
| 9 | ✅ | Testing, Sanitizers, Fuzzing, Benchmarks |
| 10 | ✅ | Documentation & Examples |

### Legacy Compatibility

The new `cppkiteconnect` namespace coexists with the legacy `kiteconnect` namespace during migration. Both can be used simultaneously, though new development should use the modern `cppkiteconnect` API.

## License

[MIT License](https://opensource.org/licenses/MIT)

---

**Contributing**: See `.github/CONTRIBUTING.md` for contribution guidelines.

**Support**: Open an issue on GitHub for bugs or feature requests.

---

## Legacy API Reference

The original `kiteconnect` namespace API is still available for backward compatibility. See the legacy examples below:

### Legacy REST API Example

```c++
#include <cstdlib>
#include <iostream>
#include "kitepp.hpp"

namespace kc = kiteconnect;

int main() {
    try {
        kc::kite Kite(std::getenv("KITE_API_KEY"));
        std::string apiSecret = std::getenv("KITE_API_SECRET");

        std::cout << "login URL: " << Kite.loginURL() << '\n';
        std::cout << "login with this URL and obtain the request token\n";

        std::string reqToken;
        std::cout << "enter obtained request token: ";
        std::cin >> reqToken;

        std::string accessToken =
            Kite.generateSession(reqToken, apiSecret).tokens.accessToken;
        Kite.setAccessToken(accessToken);
        std::cout << "access token is " << Kite.getAccessToken() << '\n';

        kc::userProfile profile = Kite.profile();
        std::cout << "name: " << profile.userName << "\n";
        std::cout << "email: " << profile.email << "\n";

} catch (kc::kiteppException& e) {
    std::cerr << e.what() << ", " << e.code() << ", " << e.message() << '\n';
} catch (kc::libException& e) {
     std::cerr << e.what() << '\n';
}
catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
};
    return 0;
};
```

### Legacy Ticker Example

```c++
#include <iostream>
#include "kitepp.hpp"

namespace kc = kiteconnect;

void onConnect(kc::ticker* ws) {
    std::cout << "Connected.. Subscribing now..\n";
    ws->setMode("full", { 408065, 2953217 });
};

void onTicks(kc::ticker* ws, const std::vector<kc::tick>& ticks) {
    for (const auto& i : ticks) {
        std::cout << "instrument token: " << i.instrumentToken
                  << " last price: " << i.lastPrice << "\n";
    };
};

void onError(kc::ticker* ws, int code, const std::string& message) {
    std::cout << "Error! Code: " << code << " message: " << message << "\n";
};

void onConnectError(kc::ticker* ws) { std::cout << "Couldn't connect..\n"; };

void onClose(kc::ticker* ws, int code, const std::string& message) {
    std::cout << "Closed the connection.. code: " << code
              << " message: " << message << "\n";
};

int main(int argc, char const* argv[]) {
    kc::ticker Ticker(std::getenv("KITE_API_KEY"), 5, true, 5);

    Ticker.setAccessToken(std::getenv("KITE_ACCESS_TOKEN"));
    Ticker.onConnect = onConnect;
    Ticker.onTicks = onTicks;
    Ticker.onError = onError;
    Ticker.onConnectError = onConnectError;
    Ticker.onClose = onClose;

    Ticker.connect();
    Ticker.run();
    Ticker.stop();

    return 0;
};
```

⚠️ **Note**: New development should use the modern `cppkiteconnect` namespace shown in the examples above. The legacy `kiteconnect` namespace uses older C++17 patterns and will be deprecated in a future release.

For more examples, see the [examples directory](https://github.com/zerodha/cppkiteconnect/tree/main/examples).

## Additional Documentation

- [Kite Connect API documentation](https://kite.trade/docs/connect/v3/)
- [CPPKiteConnect Doxygen reference](docs/html/index.html) (build with `CPPKITE_BUILD_DOCS=ON`)
- [Migration Guide](MIGRATION_PLAN.md)
- [Phase Completion Reports](docs/)
- [Risk Assessment](docs/RISK_ASSESSMENT.md)

## License

[MIT License](https://opensource.org/licenses/MIT)
