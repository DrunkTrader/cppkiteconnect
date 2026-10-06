# Repository guidance

## Implementation boundaries

- Header-only SDK: first-party implementation lives in `include/kitepp/`, not `src/`. `kite.hpp` holds REST declarations/endpoints; `kite/*.hpp` defines methods; `utils.hpp` contains HTTP/JSON/CSV helpers. Public DTOs also parse JSON in `responses/*.hpp`.
- Consumers include `include/kitepp.hpp`, `kitepp/rest.hpp` or `kitepp/ticker.hpp`. `kitepp/config.hpp` uniformly enables httplib OpenSSL and header-only fmt before dependencies. Including only a declaration header does not supply all inline definitions.
- REST uses blocking cpp-httplib; ticker owns a Beast/Asio event loop and TLS session. During `ticker::run()`, keep credentials, subscriptions and callback mutation on the loop-owning thread. Payload references are borrowed only for that invocation. `stop()` is cross-thread, idempotent and terminal; join `run()` before destruction and never delete from a callback. See `docs/runtime_contract.md`.
- `include/{PicoSHA2,cpp-httplib,fmt,rapidcsv,rapidjson,uri-parser}`, `tests/mock_responses`, and `docs/doxygen-awesome-css` are Git submodules, not first-party editing/formatting targets.

## Setup and build

- Initialize missing submodules with `git submodule update --init --recursive` (the README omits `git`); preserve recorded pins rather than updating to upstream HEAD.
- OpenSSL >=3.0 from a supported provider and Threads are required; ticker/umbrella additionally need Boost >=1.83 with a config package. Tests need GTest/GMock config targets >=1.10 (1.18 qualified), Python3 and openssl CLI for synthetic certificates. uWS/libuv/zlib are not SDK requirements. See `docs/building.md` and `docs/dependencies.md`.
- `deps/CMakeLists.txt` only downloads old archives; root CMake does not use it to build/install dependencies. Running its documented in-source command also writes into a nested `deps/deps/` path.
- CMake >=3.18 exports header-only `kitepp::models`, `kitepp::rest`, `kitepp::ticker` and `kitepp::kitepp`. Default top-level build compiles `kitepp-smoke`; tests/examples/docs/benchmarks are opt-in. Consumers receive `cxx_std_20`; repository executables use the selected required standard with extensions disabled. `KITEPP_CXX_STANDARD=17` selects the protected rollback mode and propagates `KITEPP_CPP17_COMPAT=1`; use separate build/install trees and consistent language/macros across all downstream TUs. `KITEPP_ENABLE_TICKER=OFF` avoids Boost discovery. Models still include HTTP/CSV/fmt utilities and are not transport-independent.

Run from the repository root. Tests resolve fixture paths through the CMake-provided `KITE_TEST_DATA_DIR`, so external build trees are supported:

```sh
cmake -S . -B build -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
cmake --build build
cmake -E chdir build ctest --output-on-failure
```

## Focused verification

```sh
cmake --build build --target kiteTest
cmake -E chdir build ./kiteTest --gtest_filter=kiteTest.generateSessionTest
cmake --build build --target tickerTest
cmake -E chdir build ./tickerTest --gtest_filter=tickerTest.binaryParsingTest
```

- CTest names are `kite-test`, `ticker-test`, and `ticker-lifecycle-test`, distinct from executable names. Select one with `ctest --test-dir build -R '^kite-test$' --output-on-failure`.
- REST tests use the REST-only header and production `virtual sendReq` seam; mocks bypass actual HTTP/TLS. `phase1.cpp` adds a real loopback request helper check. `tickertest.cpp` covers bounded binary/text parsing; `ticker_lifecycle.cpp` uses synthetic local TLS/WSS servers without real credentials.
- REST test sources use `GLOB CONFIGURE_DEPENDS` from `tests/unit/kite/*.cpp`. `tests/unit/main.cpp` is not in that target; linked test libraries supply main.
- Use `Boost_ROOT` or `BOOST_ROOT` for a non-system Boost provider. REST-only public inclusion does not require Boost headers.
- `KITEPP_CHECK_HEADERS=ON` builds one validation TU per first-party header. `tests/consumer` checks manual/subdirectory/installed consumption and REST/ticker multi-TU include ordering. Installed consumers need only exported targets; headers/license/config files are relocatable, with no SDK binary.

## Examples, docs and migration

- `example2.cpp` is mostly commented Doxygen snippets, so a successful example build does not validate those snippets. Runnable examples use `KITE_API_KEY`, `KITE_API_SECRET` and/or `KITE_ACCESS_TOKEN`; they are live API clients, not offline tests. `example1` prints its access token.
- Docs require Doxygen and `BUILD_DOCS`: `cmake -S . -B build -DBUILD_DOCS=ON`, then `cmake --build build --target docs`. Narrative content is duplicated in `README.md` and `docs/mainpage.md`.
- Formatting is configured by `.clang-format` (Microsoft base, 80 columns); `.clang-tidy` exists but root CMake/CI has no lint target.
- For modernization, read `audit/plan.md`, `audit/audit_report.md`, `audit/architecture.md`, and the append-only `audit/migration_log.md`. Historical findings are preserved; current fixes have migration updates. Phase 4 qualifies httplib 0.59/fmt 12.2/rapidcsv 9.07. Phase 5 C++20 passes Linux/AArch64 GCC 13/14 and Clang 18/20 checkpoints; full platform acceptance is pending. Ubuntu libc++/libc++abi 18 fails a standalone ASan exception-allocator probe; use the qualified LLVM 20 provider for libc++ sanitizers, with matching-stdlib GTest. `std::expected` and `std::byteswap` are C++23, not C++20. `KITEPP_BUILD_BENCHMARKS=ON` builds an offline synthetic decoder allocation/latency probe; it is not production performance evidence.
