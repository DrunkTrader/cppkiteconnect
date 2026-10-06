# Building and consuming the header-only SDK

The migration branch defaults to **C++20**, on 64-bit platforms, with CMake 3.18
or newer. All SDK targets remain header-only `INTERFACE` targets. Their transitive
`cxx_std_20` requirement raises a consumer's older requested standard; repository
executables require the selected standard and disable compiler extensions.
Package version 2.2.0 follows the baseline tag; it does not announce a new release.
Linux/AArch64 qualification passes; the complete compiler/OS release matrix is
still pending. See the qualification table and C++17 rollback instructions below.

## Dependencies

Initialize recorded submodules explicitly with
`git submodule update --init --recursive`. Configuration verifies bundled header
hashes and, when Git metadata is present, clean recorded revisions. It never
downloads dependencies or updates submodules. Source archives must contain the
bundled headers. SDK header providers are the pinned bundled versions; arbitrary
external fmt/JSON/CSV/httplib providers are not qualified.

OpenSSL >=3.0 from a security-supported provider and Threads are package-provided
dependencies; see [dependencies.md](dependencies.md). Ticker/umbrella additionally
require Boost >=1.83 headers (Asio/Beast/System; no compiled Boost.System required
in the qualified configuration). Use `Boost_ROOT` or `CMAKE_PREFIX_PATH` for an
external provider. uWS, libuv and zlib are not required by the default configuration.
Tests additionally need a GTest/GMock config package >=1.10, Python3 and the
openssl CLI. Doxygen and Graphviz are needed only for `BUILD_DOCS=ON`.

## Repository builds

```sh
cmake -S . -B build -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON -DKITEPP_CHECK_HEADERS=ON
cmake --build build --parallel 2
cmake -E chdir build ctest --output-on-failure
```

The top-level default build compiles `kitepp-smoke`; it does not silently build
zero SDK code. Tests/examples/docs default OFF. `KITEPP_CHECK_HEADERS` adds a
separate TU for each first-party header. `KITEPP_BUILD_SMOKE=OFF` skips the default
consumer for packaging-only work. None of these executables is an SDK library.

## Language compatibility and rebuilding

An explicit protected rollback configuration remains available:

```sh
cmake -S . -B build-cxx17 -DKITEPP_CXX_STANDARD=17 -DBUILD_TESTS=ON
cmake --build build-cxx17
cmake -E chdir build-cxx17 ctest --output-on-failure
```

Only `20` and `17` are valid SDK selections. The rollback targets propagate
`cxx_std_17` and `KITEPP_CPP17_COMPAT=1`, including through installed exports.
Use a separate build/install prefix for each configuration. Manual C++17 users
must define `KITEPP_CPP17_COMPAT=1` before any SDK header in every TU; otherwise
the configuration header diagnoses a language-standard mismatch.

Rebuild **every downstream translation unit** after changing SDK headers,
language mode, provider versions or configuration macros. Use the same language
and feature configuration across TUs that include the SDK; mixing C++17 and
C++20 ticker definitions is unsupported. There is no cross-configuration ABI
promise. The C++17 configuration remains until full C++20 matrix acceptance.

## Qualified toolchains

| Platform/provider | Verified checkpoint |
| --- | --- |
| Linux/AArch64, GCC 13.3 / libstdc++ 13 | C++20 normal and ASan/UBSan/leak suites; C++17 rollback |
| Linux/AArch64, GCC 14.2 / libstdc++ 14 | C++20 headers, examples and offline suite with distro GTest 1.14 |
| Linux/AArch64, Clang 18.1.3 / libstdc++ 13 | C++20 normal and ASan/UBSan/leak suites |
| Linux/AArch64, Clang 18.1.3 / libc++ 18 | Normal compatibility checks pass; Ubuntu 18 runtime rejected for ASan (see below) |
| Linux/AArch64, Clang 20.1.2 / libc++ 20 | C++20 headers/examples, normal and ASan/UBSan/leak suites; relocated consumers |
| CMake 3.18.4 and 4.4.4 | C++20 default smoke/relocated consumption and full/current-tool suites; minimum-tool C++17 full suite |
| macOS, AppleClang / libc++ | CI lane defined; NOT RUN locally — no macOS runner |
| Windows, other compiler/OS/provider combinations | NOT RUN — separate qualification required |

These are tested checkpoints, not promises for every compiler accepting C++20.
CI uses matching-libc++ GTest providers and the qualified libc++ 20 sanitizer
lane; hosted execution is pending. TSan cannot start on the current host (runtime
mapping failure).

Ubuntu's `libc++`/`libc++abi` 18.1.3-1ubuntu1 packages reproduce an ASan
`operator new`/`free` mismatch during nested `std::runtime_error` handling in a
standalone program without the SDK. That provider is not sanitizer-qualified.
The same unsuppressed probe and SDK suite pass with the Ubuntu 20.1.2 provider;
use that provider for libc++ sanitizer validation. No allocator-mismatch checks
are disabled. See the append-only journal for the original failure and evidence.

## add_subdirectory

```cmake
add_subdirectory(path/to/cppkiteconnect)
target_link_libraries(my_client PRIVATE kitepp::rest)
```

Set `KITEPP_ENABLE_TICKER=OFF` before adding the directory for a REST/model-only
configuration that does not discover Boost. Subdirectory builds do not enable
the smoke executable by default. Use `kitepp::ticker` for streaming or
`kitepp::kitepp` for both clients. `kitepp::models` reflects existing model
coupling to JSON/CSV/fmt/httplib/OpenSSL/Threads; it is not transport-independent.

## Installed package

```sh
cmake --install build --prefix /chosen/prefix
```

```cmake
find_package(kitepp 2.2 CONFIG REQUIRED COMPONENTS rest)
target_link_libraries(my_client PRIVATE kitepp::rest)
```

Supply the prefix via `CMAKE_PREFIX_PATH`. Components are `models`, `rest`,
`ticker`, and `kitepp`; an omitted component list selects the umbrella. REST/model
discovery does not find Boost even if the full package was installed. Ticker
components are unavailable when installed with `KITEPP_ENABLE_TICKER=OFF`.
When ticker is requested as an optional component, missing Boost leaves it
unavailable while required REST/model components remain usable. Required ticker
or umbrella components still require Boost.
Installs contain first-party headers, required bundled headers/licenses and
relative CMake exports. They can be moved without preserving the source/build
tree. OpenSSL/Boost are resolved anew on the consumer machine.

## Manual inclusion and configuration

Add `include/` to the include path and use `<kitepp/rest.hpp>`,
`<kitepp/ticker.hpp>` or `<kitepp.hpp>`. Compile with C++20. Link OpenSSL SSL/Crypto
and the platform thread dependency; add Boost headers for ticker.
`kitepp/config.hpp` establishes
`CPPHTTPLIB_OPENSSL_SUPPORT` and `FMT_HEADER_ONLY` uniformly. If including those
third-party headers first, define the same macros before them in **every** TU.
Mixed or late configuration is unsupported and diagnosed where detectable.
Additional third-party feature macros require matching configuration and links
throughout the application.

`tests/consumer` is an independent smoke project; `KITEPP_CONSUMPTION` selects
`headers`, `subdirectory` or `installed`, and `KITEPP_CONSUMER_TICKER=OFF` exercises
REST/model-only consumption. Runtime/thread/TLS rules are in
[runtime_contract.md](runtime_contract.md).

## Offline decoder probe

```sh
cmake -S . -B build-bench20 -DCMAKE_BUILD_TYPE=Release -DKITEPP_BUILD_BENCHMARKS=ON
cmake --build build-bench20 --target kitepp-decoder-benchmark
./build-bench20/kitepp-decoder-benchmark
```

For a comparison, use a separate tree with `-DKITEPP_CXX_STANDARD=17`. The probe
uses synthetic full-tick frames and validates a checksum before reporting ordinary
new/new[] allocations and mean/p50/p99 timing. It does not access a live service.
The migration journal records the observed 62/940/3728 versus 12/170/654
allocations for 1/16/64 packets in C++17 versus C++20. Timings include harness
overhead and vary on shared hosts; these are not production latency guarantees.
