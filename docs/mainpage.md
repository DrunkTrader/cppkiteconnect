# CPPKiteConnect Documentation

\tableofcontents

## Overview

CPPKiteConnect is a header-only C++ SDK for the Kite Connect REST and streaming
APIs. The current branch defaults to C++20 and exposes relocatable CMake
`INTERFACE` targets for models, REST, ticker and the combined SDK.

REST remains synchronous and uses cpp-httplib. The ticker uses Boost.Asio and
Boost.Beast with TLS peer-chain/hostname verification, SNI, cancellable connect
and retry operations, bounded queues and an explicit `connect()` → `run()`
lifecycle.

The current qualification is Linux/AArch64 with GCC 13/14 and Clang 18/20,
including libstdc++, libc++, ASan and UBSan checkpoints. macOS, Windows and other
platforms require separate qualification. The package version remains `2.2.0`
while this migration is being qualified.

## Requirements

- 64-bit platform and C++20.
- CMake 3.18 or newer.
- OpenSSL 3.0 or newer from a security-supported provider.
- Boost 1.83 or newer for ticker/umbrella targets.
- Threads and normal platform socket dependencies.
- Doxygen and Graphviz when generating API documentation.

The SDK bundles qualified header providers for cpp-httplib, fmt, rapidcsv,
RapidJSON and PicoSHA2. uWebSockets, libuv and zlib are not active SDK
requirements. Tests additionally use GTest/GMock, Python 3 and the OpenSSL CLI.

## Build and install

```sh
git submodule update --init --recursive
cmake -S . -B build -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON \
  -DKITEPP_CHECK_HEADERS=ON
cmake --build build --parallel 2
cmake -E chdir build ctest --output-on-failure
cmake --install build --prefix /chosen/prefix
```

The default build compiles an SDK smoke consumer. Tests, live examples,
benchmarks and Doxygen output are opt-in. See [building.md](building.md) for
package components, manual inclusion, sanitizers and the C++17 rollback.

## CMake components

```cmake
find_package(kitepp 2.2 CONFIG REQUIRED COMPONENTS rest)
target_link_libraries(my_app PRIVATE kitepp::rest)
```

Available targets are `kitepp::models`, `kitepp::rest`, `kitepp::ticker` and
`kitepp::kitepp`. REST/model components do not discover Boost. An optional ticker
component is ignored when Boost is unavailable; required ticker/umbrella
components remain strict.

## REST and ticker usage

The REST facade accepts the API key and access token through the existing public
API. The ticker follows this owner-thread lifecycle:

```text
configure credentials/options/callbacks
        -> connect()
        -> run()       [callbacks and mutable ticker state stay on this thread]
        -> stop()      [cross-thread stop is supported and terminal]
        -> join run()
        -> destroy ticker
```

Use `subscribe()` before `setMode()`. Reconnect replay sends explicit subscribe
commands before mode commands. Callback arguments are borrowed for the invocation;
copy values that must survive it. Never destroy a ticker from a callback or while
`run()` is active. Full rules are in [runtime_contract.md](runtime_contract.md).

Runnable examples 1, 3 and 4 use `KITE_API_KEY`, `KITE_API_SECRET` and
`KITE_ACCESS_TOKEN` as appropriate. Example2 is a compiled Doxygen snippet
collection. The runnable examples validate required environment variables and do
not print access tokens.

## C++17 compatibility mode

The protected rollback remains available until the complete platform matrix is
accepted:

```sh
cmake -S . -B build-cxx17 -DKITEPP_CXX_STANDARD=17 -DBUILD_TESTS=ON
```

Use separate build/install trees and rebuild all downstream translation units when
changing the standard or header-provider configuration.

## Decoder qualification

The opt-in decoder benchmark compares validated binary decoding under C++17 and
C++20 using synthetic 184-byte full packets. It reports allocations and timing,
but is not production-service or latency-SLA evidence:

```sh
cmake -S . -B build-bench20 -DCMAKE_BUILD_TYPE=Release \
  -DKITEPP_BUILD_BENCHMARKS=ON
cmake --build build-bench20 --target kitepp-decoder-benchmark
./build-bench20/kitepp-decoder-benchmark
```

## Further documentation

- [Build and package guide](building.md)
- [Dependency policy](dependencies.md)
- [Runtime contract](runtime_contract.md)
- [Migration release notes](release-notes.md)
- [Kite Connect API documentation](https://kite.trade/docs/connect/v3/)

## License

[MIT](https://opensource.org/licenses/MIT)
