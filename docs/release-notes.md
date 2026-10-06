# Migration release notes

## Current qualification state

This document records the release-facing changes in the C++20 migration branch.
It is not a version announcement. The CMake package version remains `2.2.0` until
the complete platform matrix and release approval are complete.

### Runtime and transport

- REST remains a blocking synchronous cpp-httplib client.
- The ticker now uses Boost.Asio and Boost.Beast instead of uWebSockets.
- TLS peer-chain and hostname verification are mandatory; SNI is configured for
  the requested host.
- Default trust roots or `tickerOptions::caFile` may be used.
- DNS, connect, TLS, write, retry and close operations are cancellable and
  bounded.
- `stop()` is terminal and idempotent. Callers must join `run()` before destroying
  a ticker.
- Ticker callbacks run on the `run()` owner thread. Payload references are borrowed
  for the callback invocation and must be copied to retain them.
- Reconnect replay sends subscribe commands before mode commands.

### Language and build contract

- C++20 is the default SDK requirement on 64-bit platforms.
- CMake 3.18 or newer is supported for package configuration.
- All SDK targets remain header-only `INTERFACE` targets.
- The explicit `KITEPP_CXX_STANDARD=17` configuration remains available as a
  protected rollback and exports `KITEPP_CPP17_COMPAT=1`.
- Consumers must rebuild all translation units after changing the language mode,
  bundled provider revisions or SDK configuration macros.
- Installed exports carry the language requirement and transitive OpenSSL,
  Threads and optional Boost closure without source/build paths.

### Dependency baseline

- cpp-httplib 0.59.0
- fmt 12.2.0
- rapidcsv 9.07
- Boost 1.83 is the locally qualified Beast/Asio floor
- OpenSSL 3.0.13 Ubuntu security-patched package in the Linux checkpoint
- GTest/GMock 1.18.0 external qualification and Ubuntu 1.14.0 provider checks
- RapidJSON and PicoSHA2 retained for public compatibility
- uWebSockets, libuv and zlib removed from the active SDK closure

See [dependencies.md](dependencies.md) for provider policy. Exact revisions and
test evidence are recorded in the repository's append-only migration journal in
the audit directory.

### Parser and correctness changes

- JSON roots, envelopes, members, containers and scalar types are checked before
  access.
- Malformed UTF-8, raw embedded NULs, invalid numeric ranges, short CSV rows,
  short candle rows and malformed binary packets are rejected deterministically.
- Collection parsing replaces destination collections and does not consume the
  source DOM through the DTO paths.
- CSV numeric parsing is locale-independent and rejects malformed, non-finite,
  overflowing and underflow-to-zero values while accepting representable
  subnormals.
- HTTP 200 error envelopes are classified as errors, with the approved mutual-fund
  compatibility exception documented in the runtime contract.

### Packaging and CI

- Relocatable component exports are available for models, REST, ticker and the
  umbrella target.
- REST/model-only installation does not require Boost.
- Optional ticker discovery tolerates missing Boost; required ticker discovery does
  not.
- Header checks, independent consumers and mixed-order multi-TU consumers are
  available through CMake options and test projects.
- CI defines C++17 rollback, C++20 GCC, Clang/libstdc++, Clang/libc++, sanitizer,
  minimum/current CMake and macOS lanes.

### Compatibility and deferred work

The high-level REST/ticker facade, DTO ownership, callback fields, fluent request
parameters and exception hierarchy remain compatibility surfaces. Strong IDs,
validated request factories, explicit optional data states, result-returning APIs
and removal of public RapidJSON coupling are deferred to a separately reviewed
major-version change.

The REST root and broad network policy remain fixed by the existing facade. Live
Kite service behavior and complete macOS/Windows release acceptance are not
established by the offline test suite.

## Downstream checklist

1. Use C++20 and a supported 64-bit compiler.
2. Rebuild every translation unit that includes the SDK.
3. Define `CPPHTTPLIB_OPENSSL_SUPPORT` and `FMT_HEADER_ONLY` consistently if
   third-party headers are included before SDK headers.
4. Provide a supported OpenSSL trust store.
5. For ticker, provide Boost headers and follow the owner-thread/run/join contract.
6. Copy callback payloads that must survive the callback invocation.
7. Do not print or commit API secrets, request tokens or access tokens.
