# Kite Connect API C++ client

CPPKiteConnect is a header-only C++ SDK for the Kite Connect REST and streaming
APIs. The current migration branch defaults to **C++20** and uses Boost.Beast and
Boost.Asio for the ticker. REST remains synchronous and uses cpp-httplib.

The SDK is qualified on Linux/AArch64 with GCC 13/14 and Clang 18/20, including
libstdc++, libc++, ASan and UBSan checkpoints. macOS, Windows and other platforms
remain separate qualification targets. The package version remains `2.2.0` while
the migration is being qualified; this repository state is not a release artifact.

## What is included

- Header-only CMake targets: `kitepp::models`, `kitepp::rest`,
  `kitepp::ticker` and `kitepp::kitepp`.
- Blocking synchronous REST requests with validated JSON/CSV boundaries.
- TLS-verified WebSocket ticker with SNI, hostname verification and configurable
  trust roots.
- Owner-thread callbacks, bounded inbound/outbound queues, cancellable retries
  and terminal `stop()`.
- Explicit subscribe-before-mode replay after reconnect.
- Relocatable CMake package exports and single-/multi-translation-unit consumer
  checks.

The legacy uWebSockets/libuv setup is no longer part of the active SDK. The ticker
uses Boost.Asio/Beast; uWS, libuv and zlib are not SDK requirements.

## Requirements

### Runtime and SDK builds

- 64-bit platform.
- C++20 and CMake 3.18 or newer.
- OpenSSL 3.0 or newer from a security-supported provider.
- Boost 1.83 or newer for `kitepp::ticker` and `kitepp::kitepp`.
- Threads and the platform's normal socket dependencies.

The SDK bundles the qualified cpp-httplib 0.59.0, fmt 12.2.0, rapidcsv 9.07,
RapidJSON and PicoSHA2 headers. Configuration verifies pinned representative
header hashes and does not download or update dependencies.

### Tests and documentation

- GTest/GMock 1.10 or newer through a CMake config package.
- Python 3 and the OpenSSL command-line tool for local TLS certificates.
- Doxygen and Graphviz only when `BUILD_DOCS=ON`.

## Get the source

```sh
git clone https://github.com/zerodha/cppkiteconnect.git
cd cppkiteconnect
git submodule update --init --recursive
```

The recorded submodule revisions are part of the qualification. Do not update
submodules to upstream HEAD as part of a normal build.

## Build and test

```sh
cmake -S . -B build \
  -DBUILD_TESTS=ON \
  -DBUILD_EXAMPLES=ON \
  -DKITEPP_CHECK_HEADERS=ON
cmake --build build --parallel 2
cmake -E chdir build ctest --output-on-failure
```

The default top-level build compiles `kitepp-smoke`; tests, examples, benchmarks
and documentation are opt-in. See [docs/building.md](docs/building.md) for
manual inclusion, package installation, component selection, sanitizer settings
and the qualified compiler matrix.

### C++17 rollback

The protected compatibility configuration remains available while the complete
platform matrix is being qualified:

```sh
cmake -S . -B build-cxx17 -DKITEPP_CXX_STANDARD=17 -DBUILD_TESTS=ON
cmake --build build-cxx17
cmake -E chdir build-cxx17 ctest --output-on-failure
```

Use separate build/install trees for C++17 and C++20. All downstream translation
units must use the same SDK language and configuration macros.

## Install and consume with CMake

```sh
cmake --install build --prefix "$HOME/.local/kitepp"
```

```cmake
find_package(kitepp 2.2 CONFIG REQUIRED COMPONENTS rest)
target_link_libraries(my_app PRIVATE kitepp::rest)
```

Use `COMPONENTS ticker` for streaming or `COMPONENTS kitepp` for the umbrella.
REST/model discovery does not require Boost. If ticker is optional during package
discovery, missing Boost leaves core components usable; required ticker components
still fail with a dependency diagnostic.

## Minimal REST usage

The REST API needs credentials obtained through the Kite login flow. This example
uses an existing access token and deliberately does not print credentials:

```cpp
#include <cstdlib>
#include <iostream>
#include <kitepp/rest.hpp>

int main() {
    const char* key = std::getenv("KITE_API_KEY");
    const char* token = std::getenv("KITE_ACCESS_TOKEN");
    if (!key || !token) {
        std::cerr << "KITE_API_KEY and KITE_ACCESS_TOKEN are required\n";
        return 2;
    }

    kiteconnect::kite client(key);
    client.setAccessToken(token);
    const auto profile = client.profile();
    std::cout << profile.userName << " <" << profile.email << ">\n";
}
```

For interactive login, call `loginURL()`, obtain the request token externally,
then pass it to `generateSession()` with the API secret. Store the resulting
access token using the application's secret-management mechanism.

## Minimal ticker usage

`connect()` and `run()` execute on the owner thread. Configure credentials and
callbacks before entering `run()`; call `stop()` from another thread or a callback
when the application is finished.

```cpp
#include <cstdlib>
#include <iostream>
#include <kitepp/ticker.hpp>

int main() {
    const char* key = std::getenv("KITE_API_KEY");
    const char* token = std::getenv("KITE_ACCESS_TOKEN");
    if (!key || !token) { return 2; }

    kiteconnect::ticker client(key);
    client.setAccessToken(token);
    client.onConnect = [](kiteconnect::ticker* ticker) {
        ticker->subscribe({408065});
        ticker->setMode(kiteconnect::MODE_FULL, {408065});
    };
    client.onTicks = [](kiteconnect::ticker*,
        const std::vector<kiteconnect::tick>& ticks) {
        for (const auto& tick : ticks) {
            std::cout << tick.instrumentToken << ': '
                      << tick.lastPrice << '\n';
        }
    };
    client.connect();
    client.run();
}
```

Payload references are borrowed for the callback invocation; copy data that must
outlive the callback. Destroy a ticker only after `run()` returns and its owning
thread has joined. See [docs/runtime_contract.md](docs/runtime_contract.md).

## Examples, benchmarks and documentation

Examples 1, 3 and 4 are live API clients; example2 is a compiled Doxygen snippet
collection. They are compiled by `BUILD_EXAMPLES=ON` and do not run as offline
tests. They use `KITE_API_KEY`, `KITE_API_SECRET` and/or `KITE_ACCESS_TOKEN` and
never print access tokens.

Generate API documentation with:

```sh
cmake -S . -B build-docs -DBUILD_DOCS=ON
cmake --build build-docs --target docs
```

Compare the decoder's C++17/C++20 allocation behavior with the opt-in benchmark:

```sh
cmake -S . -B build-bench20 -DCMAKE_BUILD_TYPE=Release \
  -DKITEPP_BUILD_BENCHMARKS=ON
cmake --build build-bench20 --target kitepp-decoder-benchmark
./build-bench20/kitepp-decoder-benchmark
```

The benchmark uses synthetic frames and is not production latency evidence.

## Known scope and compatibility

The current public REST/DTO/exception surfaces remain compatibility-oriented.
Major API changes such as strong identifiers, result-returning interfaces,
explicit optional states and removal of public RapidJSON coupling are deferred to
a future major-version review. The REST root and broader network policy are still
fixed by the existing facade. Live-service behavior and complete macOS/Windows
release acceptance are not claimed by the offline qualification suite.

See:

- [Build and package guide](docs/building.md)
- [Runtime contract](docs/runtime_contract.md)
- [Dependency policy](docs/dependencies.md)
- [Migration release notes](docs/release-notes.md)
- [Migration audit and qualification journal](audit/migration_log.md)

## License

[MIT](LICENSE)
