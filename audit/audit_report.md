# cppkiteconnect — Engineering Audit

**Audit date:** 2026-10-05  
**Repository:** `/home/drunktrader/cppkiteconnect`  
**Inspected revision:** `d437b35300bfd13f9de10632aeb19410720c8ad7` (2023-11-17, basket margin endpoint).  
**Scope:** read-only inspection of the project, initialized dependency submodules, tests, examples, build configuration, and official upstream information. Only the two audit documents were added to the repository.

Throughout this report:

- **Current** describes implemented behavior.
- **Finding** identifies an evidenced defect, limitation, or technical characteristic.
- **Recommendation** describes subsequent work; no fixes were made here.
- **Future** describes a proposed architecture, never the current implementation.
- **BLOCKER** means required before/during a trustworthy migration or release, not necessarily a compiler error. **HIGH**, **MEDIUM**, and **LOW** express descending engineering priority.
- **Confirmed** means directly established by source or a diagnostic. **Likely** and **Possible optimization** identify conclusions requiring additional measurement or integration testing.

**Location convention:** abbreviated SDK paths (`kite/...`, `ticker/...`, `responses/...`, `utils.hpp`, `exceptions.hpp`, `userconstants.hpp`) are relative to `include/kitepp/`. Other project paths are relative to the repository root. Dependency-header paths are identified by their submodule name.

## 1. Executive Summary

**Current:** cppkiteconnect is a C++17 header-only SDK with two independently configured clients: `kiteconnect::kite` for blocking REST requests through cpp-httplib, and `kiteconnect::ticker` for callbacks on a legacy uWS v0.14 event loop. Response structs parse RapidJSON objects themselves. CMake builds examples and two test executables but defines no SDK library target, installation, or exported package.

**Finding:** changing `CMAKE_CXX_STANDARD` to 20 is insufficient. A real C++20 compilation failure exists at runtime format-string calls to fmt 9.1. The larger release risks are pre-existing correctness and security problems: dangling error-helper captures, unchecked binary/JSON/CSV reads, malformed subscription JSON, incomplete resubscription, unowned uWS groups/timers, and missing WebSocket certificate authentication in the documented external implementation.

**Recommendation — what to change first:** restore a reproducible C++17 baseline and strengthen offline behavioral tests before changing the standard or transport. Immediately prioritize parser lifetime/bounds, subscription serialization, authentication state, and TLS/shutdown contracts. Then modernize build usage requirements, refresh dependencies independently, and introduce C++20 in a separate, behavior-preserving change.

| Priority | Principal issue | Evidence |
|---|---|---|
| BLOCKER | C++20 runtime format strings rejected by fmt | `kite/api.hpp:45`; `ticker/internal.hpp:170`; GCC diagnostic |
| BLOCKER | Insufficient transport/lifecycle/malformed-input tests | 41 mocked REST tests, one happy-path binary test; §16 |
| BLOCKER | WebSocket TLS does not authenticate server in documented uWS implementation | `ticker::connectInternal`; upstream `uS::Node` source; F26 |
| HIGH | JSON diagnostic closure references expired stack parameter | `utils.hpp:146-148,220-222,247-249`; ASan reproduction |
| HIGH | Binary and structured data paths trust lengths/types | F16 |
| HIGH | Subscription/unsubscription action becomes JSON null | `utils::json::json::field`; `ticker::subscribe/unsubscribe`; runtime reproduction |
| HIGH | Streaming shutdown, reconnect, and replay are incomplete | F10–F14 |
| HIGH | Build cannot be consumed as a reliable CMake SDK package | F01–F03 |

**Verification limits:** GCC 13.3 on Linux/aarch64 compiled the REST implementation headers in C++17 and rejected them in C++20. Offline probes and an ASan probe were run. CMake, Clang, uWS headers, and libuv development discovery were unavailable; the existing CMake/CTest suite and live trading/TLS integration were not run. This is a source-grounded audit with targeted diagnostics, not a certification of all platforms or dependencies.

## 2. Repository Overview

### Current — meaningful tree

```text
cppkiteconnect/
├── CMakeLists.txt                   # examples, tests, docs; standard/dependency discovery
├── Dockerfile                       # Fedora 38 image, external uWS build, examples
├── README.md, LICENSE
├── .gitmodules, .gitignore
├── .clang-format, .clang-tidy, .git-blame-ignore-revs
├── .github/
│   ├── workflows/cppkiteconnect-test.yml
│   └── ISSUE_TEMPLATE/bug_report.md
├── cmake/
│   ├── modules/FindGMock.cmake
│   └── templates/Doxyfile.in
├── deps/CMakeLists.txt              # separate archive downloader, not a dependency build
├── include/
│   ├── kitepp.hpp                   # supported umbrella; enables httplib OpenSSL
│   ├── kitepp/
│   │   ├── kite.hpp                 # REST class, endpoint table, per-instance state
│   │   ├── kite/{api,gtt,internal,kite,margins,market,mf,order,portfolio,user}.hpp
│   │   ├── ticker.hpp
│   │   ├── ticker/{ws,internal}.hpp  # ticker declarations and implementation
│   │   ├── responses/{gtt,margins,market,mf,order,portfolio,user,ws,responses}.hpp
│   │   └── exceptions.hpp, userconstants.hpp, utils.hpp
│   └── PicoSHA2/, cpp-httplib/, fmt/, rapidcsv/, rapidjson/, uri-parser/
│                                    # pinned Git submodules, full upstream source trees
├── examples/example{1,2,3,4}.cpp
├── tests/
│   ├── unit/{kitepp,utils}.hpp, main.cpp, tickertest.cpp
│   ├── unit/kite/{api,gtt,margins,market,mf,order,portfolio,user}.cpp
│   ├── mock_custom/                 # three synthetic JSON fixtures and binary ticks
│   └── mock_responses/              # pinned Zerodha fixture submodule
├── docs/
│   ├── mainpage.md, header.html
│   └── doxygen-awesome-css/         # pinned documentation theme submodule
└── audit/                           # this audit's two deliverables
```

There is no project `src/` directory, root Makefile, benchmark suite, standalone script directory, Conan/vcpkg manifest, package lock, generated SDK source, or compiled SDK artifact. Makefiles, CI, examples, tests, and nested third-party GoogleTest under vendor trees belong to those upstream projects and are not built by the root CMake. Generated Doxygen output and compile databases are build artifacts, not supplied SDK implementation. No `AGENTS.md`, `CONVENTIONS.md`, or project `.editorconfig` was found. The initial Git index/worktree was clean and all eight actual submodules matched their recorded gitlinks.

### Finding F00 — duplicate submodule metadata

- **Severity:** LOW.
- **Location:** `.gitmodules:16-21`.
- **Finding:** an obsolete `doxygen-awesome-css` root entry coexists with the actual `docs/doxygen-awesome-css` gitlink.
- **Why it matters:** dependency inventory and submodule maintenance have two names for one intended asset.
- **Evidence:** `git submodule status` lists only `docs/doxygen-awesome-css`; no root theme gitlink exists.
- **Recommendation:** reconcile metadata in the later build/documentation cleanup.
- **Migration impact:** non-breaking housekeeping; no runtime consequence.

## 3. Current Build System

### Current

`CMakeLists.txt:1-118` requires CMake 3.10, runs `project(CppKiteConnect)` (implicitly enabling C and C++), sets global `CMAKE_CXX_STANDARD=17`, enables compile-command export, and globally adds `${CMAKE_SOURCE_DIR}/include`. The three `BUILD_*` switches are consumed through `if(...)` but are not declared as project options; absent switches effectively build no SDK code.

| Configuration | Behavior |
|---|---|
| Default | Configure project/compiler checks; no SDK compile target |
| `BUILD_EXAMPLES` | Four executables; all link Threads, OpenSSL SSL/Crypto, zlib, uWS, and sometimes libuv |
| `BUILD_TESTS` | `kiteTest` from globbed REST tests and `tickerTest`; two CTest registrations |
| `BUILD_DOCS` | Find Doxygen; configure a build-tree Doxyfile; explicit `docs` custom target |

OpenSSL, Threads, ZLIB, uWS library/header discovery occurs only if examples or tests are enabled. Any Unix non-Apple platform is labeled Linux. libuv is optional under that label and required otherwise. GTest/GMock are discovered externally, using legacy variables and a copied find module. `CMAKE_USE_WIN32_THREADS_INIT` and `gtest_disable_pthreads` are set after discovery; this does not rebuild or configure externally installed GoogleTest.

There are no project warning flags, sanitizer options, optimization overrides, explicit debug/release policy, architecture flags, standard-required setting, or extension prohibition. Debug/release optimization follows the selected generator/toolchain and `CMAKE_BUILD_TYPE`; CI specifies no build type. There is no SDK static/shared target, `BUILD_SHARED_LIBS` behavior, install rule, export target, package config, version, SOVERSION, or public compile/link usage-requirement model.

The separate `deps/CMakeLists.txt` only downloads archives. It is not included by root CMake and neither compiles dependencies nor makes CMake find them. Its destination paths use `${CMAKE_SOURCE_DIR}/deps/...`; following README's `cmake .` *inside* `deps/` targets a nested `deps/deps/` directory rather than the documented directory. The downloader was not executed.

### Current — normal build invocation and CI/CD

With external prerequisites and initialized submodules already supplied, the existing build can be invoked as follows (documented here, not executed during this audit):

```sh
cmake -S . -B build -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON -DBUILD_DOCS=ON
cmake --build build
ctest --test-dir build --output-on-failure
cmake --build build --target docs
```

The repository-child `build/` layout is currently necessary for test fixture paths; it is not a general CMake requirement. A library consumer still supplies C++17, SDK include path and applicable transport/TLS link libraries manually. Neither configuring nor copying headers installs an exported package.

`.github/workflows/cppkiteconnect-test.yml` triggers on pushes/PRs targeting `main`/`v2` and release events. One matrix job runs Ubuntu and macOS, initializes submodules, installs system dependencies, builds external uWS, then builds examples/tests and runs verbose CTest. There is no explicit compiler-version/standard matrix, sanitizer/coverage/lint job, docs deployment, SDK packaging, artifact upload, tag-version validation, or publishing step. Release events run the same build/test job, not a release pipeline. Windows is commented out. Workflow/bootstrap fragility is source-established (F03/F04); no current remote CI-run result was retrieved, so this report does not assert that a particular hosted run failed.

### Finding F01 — no consumable SDK target or transitive requirements

- **Severity:** HIGH.
- **Location:** `CMakeLists.txt:5-16,48-96`; `README.md:54`.
- **Finding:** consumer projects must manually supply include paths, compile configuration, and binary dependencies. `add_subdirectory` is fragile because project paths use `CMAKE_SOURCE_DIR` rather than this project's directory.
- **Why it matters:** a successful default configure proves no SDK header compiles; consumers can silently use a different standard, TLS macro state, or networking ABI.
- **Evidence:** only `add_executable` and the docs custom target exist; there is no `add_library`, `target_compile_features`, or install/export block.
- **Recommendation:** introduce namespaced INTERFACE targets for REST and ticker plus an umbrella target; express standard and TLS/dependency requirements per target; use project-relative paths; add install/export consumer smoke tests.
- **Migration impact:** additive/non-breaking if manual inclusion stays supported; changing installed layout is potentially breaking.

### Finding F02 — stated CMake baseline is inconsistent with used/future features

- **Severity:** BLOCKER for the declared C++20 build contract.
- **Location:** `CMakeLists.txt:1,5,25-33`.
- **Finding:** CMake 3.10 predates `CXX_STANDARD=20` support (3.12); `find_library/find_path(... REQUIRED)` also postdates the minimum (3.18). Standard fallback is not disabled.
- **Why it matters:** oldest declared tooling cannot implement the intended migration and dependency-required behavior reliably.
- **Evidence:** official CMake property/command documentation [U1]; global standard lacks `CMAKE_CXX_STANDARD_REQUIRED`/target feature requirements.
- **Recommendation:** choose and test a realistic minimum, e.g. 3.24+ without modules; use `cxx_std_20`, required standard, and no GNU extensions on SDK validation targets. Test with current CMake 4.x too.
- **Migration impact:** definitely drops older build-tool support; need not break C++ API.

### Finding F03 — dependency discovery cannot guarantee the external uWS build contract

- **Severity:** HIGH.
- **Location:** `CMakeLists.txt:9-43,79-93`; `.github/workflows/cppkiteconnect-test.yml:37-52`.
- **Finding:** finding `uWS` and optionally `uv` does not verify uWS API version, epoll/libuv backend, thread-safe flags, ABI, or transitive links. `kiteTest` does not receive `${UWS_INCLUDE}`, although its umbrella headers include uWS.
- **Why it matters:** a non-system uWS include path works for ticker/examples but can fail REST tests. Presence of libuv is not proof it matches how uWS was compiled. BSD is misclassified as Linux.
- **Evidence:** header inclusion `include/kitepp.hpp:41` → `ticker/ws.hpp:51`; `kiteTest` include list contains only GTest/GMock variables.
- **Recommendation:** use a versioned imported networking target/config package; isolate REST-only header tests from ticker; explicitly model supported platforms and backend options.
- **Migration impact:** mostly build-level, potentially breaking previously tolerated custom installations.

### Finding F04 — reproducibility and obsolete bootstrap paths

- **Severity:** HIGH.
- **Location:** `deps/CMakeLists.txt:4-48`; `Dockerfile:1,7-10`; CI workflow `:28,37,46-52`.
- **Finding:** Linux/Docker clone moving uWS `master`; macOS uses v0.14.8; the archive downloader combines a moving master tarball with a fixed SHA256 and skips checking existing archives. Bootstrap pins OpenSSL 1.1.1i, zlib 1.2.11, libuv 1.40.0, and GoogleTest 1.10.0. Fedora 38 and historical Homebrew tap/path instructions are stale.
- **Why it matters:** builds are not reproducible and may fail or install unsupported software independently of C++ language changes.
- **Evidence:** exact URLs/pins in the cited files; external uWS master showed September 2026 changes [U5]; OpenSSL support policy [U6].
- **Recommendation:** freeze the baseline to immutable revisions; use supported system packages/config targets or one opt-in locked package mechanism; refresh CI runner/toolchain setup and containers. Validate downloaded hashes every time and use binary-tree destinations if a downloader is retained.
- **Migration impact:** dependency ABI rebuilds and new build-tool/platform requirements; stage separately from the language change.

## 4. Current C++ Standard

### Current — actual feature inventory

| Feature | Actual project usage |
|---|---|
| C++17 | `std::optional` request fields; nested namespaces; `if constexpr`; `_v` traits; inline test constants |
| C++11/14 facilities | Range-for, lambdas, `auto`, enum class, defaulted/explicit constructors, `std::function`, move into credentials/exceptions, compile-time constants, `[[nodiscard]]` |
| RAII | Value strings/vectors/maps, automatic RapidJSON documents and httplib client, value uWS Hub |
| Raw pointers | `group`, `ws`, callback `ticker*`, incoming `char*`, optional RapidJSON output pointer |
| Manual allocation | No project-owned explicit `new/delete/malloc/free`; external `createGroup` returns allocated storage not released by the wrapper |
| Smart pointers | None directly in SDK; httplib owns its implementation with `unique_ptr` internally |
| Concurrency | One `std::atomic<bool>`; `std::this_thread::sleep_for`; chrono wall-clock timestamps; no SDK thread creation or mutex |
| Absent | Structured bindings, concepts, ranges, `std::span`, `std::string_view`, `std::variant`, `std::expected`, coroutines, filesystem, `std::jthread`, modules, consteval/constinit, designated initializers |

This is a **mixture**: substantial C++17 generic/value-oriented code, but assertion-driven parsing, macros, legacy transport ownership, stringly typed domain options, and no explicit concurrency contract. Absence of an optional modern feature is not itself a defect.

### Finding F05 — confirmed C++20 fmt integration failure

- **Severity:** BLOCKER (compiler error).
- **Location:** `include/kitepp/kite/api.hpp:44-46`; `include/kitepp/ticker/internal.hpp:169-172`; `utils.hpp:77`; fmt `core.h:263-272,3148`.
- **Finding:** `FMT` maps to `fmt::format`; `loginUrlFmt` and `connectUrlFmt` are non-constexpr instance `std::string`s. fmt 9.1 enables consteval format-string checks in C++20.
- **Why it matters:** simply enabling C++20 makes a supported include fail before application logic is compiled.
- **Evidence:** GCC 13.3 `-std=c++17` syntax check of REST implementation passes; the same `-std=c++20` reports `api.hpp:45: error: 'this' is not a constant expression`. A separate dynamic-format probe fails identically. fmt documentation explicitly requires `fmt::runtime` for runtime strings [U3]. Ticker's analogous call is source-identified, not compiled locally because uWS is absent.
- **Recommendation:** later use a compile-time literal/constant where the format is fixed, or explicit runtime formatting where genuinely dynamic; keep fmt and fix integration rather than suppressing consteval globally.
- **Migration impact:** private implementation fix, non-breaking; updating fmt alone does not resolve it.

### Recommendation — useful C++20 boundaries

Use `std::span<const std::byte>` for validated binary views and `std::endian` for portable byte-order detection; concepts can improve parser/encoder template diagnostics. `std::string_view` is useful for internal immutable names and synchronous input views, but retained async buffers must own storage. Prefer `std::chrono::steady_clock` for deadlines. `std::jthread`/stop tokens only help if a worker-thread API is deliberately introduced; they do not replace socket cancellation or event-loop serialization. `std::byteswap` and `std::expected` are **C++23**, not C++20. Modules, coroutines, ranges rewrites, and constinit are not prerequisites for this SDK's migration.

## 5. Dependency Inventory

### Current — direct SDK and tool dependencies

Git submodule pins are exact commits, even when a tag is listed below. External installed versions are not fixed by root CMake; archive versions describe only the optional downloader, not necessarily runtime binaries.

| Dependency | Repository pin / resolution | Actual purpose and use |
|---|---|---|
| cpp-httplib | v0.11.4, `7992b148969fbf8093230b37a6a36c2f61135937`; Git submodule | `kite::client`, `http::Params`, blocking HTTPS in `utils.hpp` |
| fmt | 9.1.0, `a33701196adfad74917046096bf5a2aa0ab0bb50`; submodule, header-only macro | URL/path formatting, diagnostics; `utils.hpp:38-40,77,424-429` |
| RapidJSON | post-1.1.0 snapshot `1ce516e50bec548eb3273e5b8563d97a18ba233c` (724 commits past tag) | DOM parsing/serialization and public DTO parsing signatures |
| rapidcsv | v8.69, `a4877fedd4036f4812263597eec7c84f33e82675`; submodule | instrument CSV dumps through `parseInstruments` |
| PicoSHA2 | `27fcf6979298949e8a462e16d09a0351c18fcaf2` (33 commits past v1.0.0) | session checksum SHA-256, `kite/api.hpp:61-62` |
| uri-parser | `4de2d85322156ab4419550205280f9a3b8beb449` (one past v1.0.0) | percent-encode the tradingsymbol portion of quotes; `kite/internal.hpp:41-56` |
| uWebSockets/uWS | external v0.14 API; Linux/Docker hoytech master, macOS v0.14.8 | Hub/Group/WebSocket; TCP/TLS/WebSocket event engine |
| OpenSSL | external `find_package`; optional archive 1.1.1i | HTTPS and WSS TLS; SDK hashing does not use OpenSSL directly |
| zlib | external `find_package`; archive 1.2.11 | uWS compression/link dependency; httplib zlib support macro is not enabled by SDK |
| libuv | external optional Linux/required other platforms; archive 1.40.0 | external uWS event-loop backend; no direct SDK libuv calls |
| Threads / platform sockets | `Threads::Threads`; libc/Winsock via transports | request mutexes/networking/event-loop backend |
| GoogleTest/GoogleMock | external discovery; downloader/macOS workflow 1.10.0 | unit assertions and virtual `sendReq` mocking |
| kiteconnect-mocks | `02d0831c44d30a9f4ac647a3a2c89b3c0279f317`; submodule | offline Zerodha JSON/CSV/packet fixtures |
| CMake/CTest | declared minimum 3.10; system tool | project targets and two test registrations |
| Doxygen | system, unpinned | optional generated API documentation |
| doxygen-awesome-css | v2.1.0, `a3c119b4797be2039761ec1fa0731f038e3026f6` | optional docs stylesheet and dark-mode JS |
| Git, Make, OS package tools, Docker | host tools / workflow / Fedora 38 image | submodules, external uWS bootstrap, example execution |
| actions/checkout | workflow `@v2` | CI checkout and initialized submodules |
| clang-format/clang-tidy | config files, no pinned tools/CI invocation | development style/lint configuration only |

There is no runtime logging, CLI argument-parser, database/storage, metrics, test coverage, benchmark, coroutine, or standalone task-scheduler dependency. Upstream submodules ship their own build/test-only dependencies (including RapidJSON's bundled GoogleTest); root CMake does not consume those targets. That distinction prevents counting vendor test tooling as SDK runtime requirements.

## 6. Dependency Health

### Current — official upstream snapshot

Sources were retrieved on 2026-10-05 using the agent-reach web-reader route after GitHub CLI reported missing authentication. Version statements below refer to observed official pages, not guessed releases. No software was installed/updated. The agent-reach update check could not run because its executable was unavailable.

| Dependency | Official upstream observation | C++20/portability/security assessment | Recommendation |
|---|---|---|---|
| cpp-httplib | latest page v0.59.0; ongoing fixes; current docs include WebSocket client [U2] | C++11-capable, hence usable in C++20; current OpenSSL backend requires 3.0+; upstream explicitly declines 32-bit support/security review. Release security fixes are not all relevant to this client-only usage. | Upgrade in place with transport tests; don't assume a new HTTP library is necessary. |
| fmt | 12.2.0; explicit C++20/module work [U3] | Well-maintained; old integration, not fmt obsolescence, causes F05. API/header organization changed across majors. | Retain, refresh deliberately. |
| RapidJSON | latest release page still v1.1.0; repository has later development [U4] | Supplied post-tag snapshot passed the C++17 REST syntax check; no standalone C++20 incompatibility established. Slow tagged-release cadence complicates supported-version/security tracking. | Harden wrapper first; evaluate a replacement only after removing public JSON coupling. |
| uWS v0.14 / hoytech fork | fork has Sept. 2026 activity; modern mainline advertises server `App` API [U5] | Do **not** call the fork abandoned. Legacy client API and documented TLS/lifetime behavior are the concern; installed build flags/revision unspecified. Current mainline is not a drop-in `Hub::connect` client replacement. | Replace or explicitly maintain a verified legacy adapter; evaluate Beast/Asio and current httplib WS (§21). |
| OpenSSL | 4.0.3 stable; 3.5.9 LTS through 2030-04-08; 1.1.1/3.0 out of ordinary support [U6] | C API unaffected by C++20 syntax; provider/API/ABI transition from 1.1.1 must be tested. Distribution backports may differ from upstream lifecycle. | Keep TLS technology; target supported 3.5 LTS, not automatic newest-major adoption. |
| zlib | 1.3.2, 2026-02-17, audit/security fixes [U7] | C ABI; portable. Old 1.2.11 archive is stale, but actual system library may be patched. | Keep if transport requires it; update resolution, remove if unused after uWS removal. |
| libuv | v1.53.0 stable, 2026-09-24 [U8] | Maintained C API, portable; current SDK does not depend on its API directly. | Keep current supported version only while needed by selected transport; don't add Asio plus independent libuv unnecessarily. |
| rapidcsv | v9.07; limits, unsigned conversion, short-row fixes [U9] | Header-only; pinned use passed C++17 syntax check. C++20-friendly; unsafe positional indexing is SDK responsibility. | Retain and upgrade with CSV fixtures; a new CSV parser is unjustified. |
| PicoSHA2 | official minimal dependency-free SHA-256 documentation [U10] | Supplied header compiles in C++17; low update frequency for a small stable hash is not proof of insecurity. Upstream maintenance cadence/latest patch not established by retrieved page. | Keep initially; optionally consolidate into already-required OpenSSL EVP after golden checksum tests. |
| uri-parser | latest visible commit Jan. 2023, C++17 requirements [U11] | Very small custom URL codec; signed-char non-ASCII encoding failure was reproduced (F31); SDK leaves several query/path inputs unencoded. | Replace the narrow encoder with transport-owned encoding or Boost.URL if Boost is already selected. |
| GTest/GMock | v1.18.0, C++17 minimum [U12] | Actively maintained; C++20 compatible; config targets eliminate copied find-module maintenance. | Retain, upgrade; no Catch2/doctest switch needed. |
| fixtures | official repository updated March 2026, no releases [U13] | No runtime/security library; newer API examples exist beyond supplied pin. | Retain, curate pinned fixtures and add missing negative cases. |
| CMake | 4.4.4 current; 3.31.12 legacy on download page [U1] | Modern target-based C++20 build is supported; not a reason to require latest 4.x as minimum. | Raise tested minimum, retain toolchain. |
| Doxygen/theme | 1.18.0 download; theme v2.5.0 [U14] | Documentation-only; custom header is labeled Doxygen 1.9.1, so compatibility needs visual verification. | Keep; refresh paired docs tooling after runtime stabilization. |
| checkout action | v7.0.1 observed [U15] | Old v2 bootstrap is stale. Current action's runtime/self-hosted-runner requirements need review before upgrade. | Update to a supported, pinned revision compatible with CI runners. |

No comprehensive vulnerability database scan or exploitability assessment of every transitive component was performed. Unsupported bootstrap versions and directly observed TLS/parser defects are evidence-backed findings; age alone is not a CVE finding.

## 7. Architecture Findings

### Current

The architecture is reconstructed in [architecture.md](architecture.md), with actual symbols and three flow diagrams. `kite` owns a fixed-root httplib client, an instance endpoint map, API key/token, and a cached Authorization string. `ticker` owns a Hub value and stores raw Group/socket pointers, callbacks, mode/subscription map, reconnect state, and timestamps. They do not share auth objects, transport configuration, error policies, or an executor.

`utils.hpp` combines formatting, byte-order macros, JSON traits/parsing/writing, HTTP transport and envelope decoding, WebSocket codes, form conversion, and CSV conversion. Response headers include it and expose RapidJSON constructors/parsers. Even model-only includes therefore pull in HTTP, CSV, fmt, and JSON implementations.

### Finding F06 — public models and test seams expose implementation dependencies

- **Severity:** MEDIUM.
- **Location:** `responses/*.hpp`; `utils.hpp:28-46`; `kite.hpp:713-727`; `include/kitepp.hpp:26-42`.
- **Finding:** RapidJSON types appear in public constructors/`parse`; httplib types appear in protected `sendReq`; `KITE_UNIT_TEST` changes `kite` layout by making `sendReq` virtual. The supported umbrella also forces REST consumers to find uWS.
- **Why it matters:** JSON/transport replacements can break source compatibility, increase compile cost, and create ODR violations if test-mode and ordinary class definitions enter one program. There is no stable precompiled SDK ABI.
- **Evidence:** `mockKite` derives from `kite` (`tests/unit/utils.hpp:47-54`); macro wrapper `tests/unit/kitepp.hpp:28-30`; umbrella includes ticker unconditionally.
- **Recommendation:** decouple DTOs from codecs, add explicit transport injection, split public REST/ticker/model headers, and keep compatibility adapters/deprecations if JSON parsing methods remain public.
- **Migration impact:** additive headers are non-breaking; removing RapidJSON signatures or changing inheritance/layout is definitely breaking and requires a versioned release/rebuild.

## 8. API Design

### Current — public surface inventory

All SDK headers were read. The supported include is `kitepp.hpp`; namespace is `kiteconnect`. There are no export/visibility macros or compiled ABI boundaries.

| Surface | Public operations/models |
|---|---|
| REST credentials/session | constructor, `set/getApiKey`, `set/getAccessToken`, `loginURL`, `generateSession`, `invalidateSession` |
| User | `profile`, both `getMargins` overloads; user/profile/token/margin DTOs |
| Orders | `placeOrder`, `modifyOrder`, `cancelOrder`, `orders`, `orderHistory`, `trades`, `orderTrades`; request DTOs, `order`, `trade` |
| GTT | `placeGtt`, `triggers`, `getGtt`, `modifyGtt`, `deleteGtt`; condition/request/trigger DTOs |
| Portfolio | `holdings`, `getPositions`, `convertPosition`; holdings/positions/request DTOs |
| Market | `getInstruments`, `getQuote`, `getOhlc`, `getLtp`, `getHistoricalData`; quote/depth/CSV/candle/request DTOs |
| Mutual funds | place/cancel/list/get orders; holdings; place/modify/cancel SIP, list/get SIP, instruments; corresponding DTOs |
| Order/basket margins | `getOrderMargins`, `getBasketMargins`; `marginsParams`, `orderCharges`, `orderMargins`, `basketMargins` |
| Streaming | constructor, key/token getters/setters, `connect`, `isConnected`, `getLastBeatTime`, `run`, `stop`, `subscribe`, `unsubscribe`, `setMode`; nine writable `std::function` callbacks; `tick`, `depthWS`, `postback` |
| Errors/constants | abstract `kiteppException`, eight named API types plus unknown, separate `libException`; namespace string constants; public parse methods/fluent setters |

REST getters return credential copies; methods return owning DTOs/collections by value (normally appropriate). Request inputs are mostly const references. Domain options are unrestricted strings and quantities/tokens/times use inconsistent integer/double types. Public structs are mutable and fluent setters return references to `*this`; keeping a reference to a chained temporary past its full expression dangles. Client copying/moving is implicitly constrained by httplib/Hub/atomic members, rather than an explicit contract. Ticker's atomic prevents ordinary copy/move; avoid retrofitting moves while callbacks capture its address.

### Finding F07 — some ordinary default constructions leave required scalars indeterminate

- **Severity:** HIGH.
- **Location:** `responses/order.hpp:69`; `responses/gtt.hpp:110-111`; `responses/mf.hpp:70-72`; `responses/ws.hpp:65`.
- **Finding:** `placeOrderParams p;`, `modifyGttParams p;`, and `placeMfSipParams p;` leave required numeric members uninitialized. Corresponding API methods read them without validating completeness. A plain default-constructed `tick` has an uninitialized `isTradable`.
- **Why it matters:** users who omit a fluent setter or read a default tick can trigger undefined/indeterminate behavior; a trading request should fail deterministically rather than use arbitrary quantity/amount values.
- **Evidence:** no initializers on the cited fields; `placeOrder` stringifies quantity, `getConditionJson` reads lastPrice, `placeMfSip` stringifies amount/installments. Value initialization with `{}` may zero-initialize these, so the risk depends on construction style.
- **Recommendation:** define safe initialization plus validation of required fields/ranges; later consider validated request factories/types.
- **Migration impact:** initializing fields is source-compatible; rejecting invalid requests changes behavior; immutable/strong request types are definitely breaking if substituted.

### Finding F08 — credential mutation leaves cached Authorization stale

- **Severity:** HIGH.
- **Location:** `kite/api.hpp:40,48-51`; `kite/internal.hpp:39`; `kite.hpp:707-710`.
- **Finding:** after `setAccessToken`, calling `setApiKey` changes `key` but not `authorization`. Subsequent REST requests still send the old key/token header, while getters/login/session paths use the new key.
- **Why it matters:** switching client credentials produces inconsistent authentication and hard-to-diagnose failures.
- **Evidence:** only `setAccessToken` builds `authorization`; `sendReq` obtains it from `getAuth`.
- **Recommendation:** centralize credentials and derive auth from coherent state, or rebuild on either setter. Specify token reset/invalidation semantics.
- **Migration impact:** private fix/non-breaking API; behavioral correction should have a credential-switch regression test.

### Recommendation — compatibility classification

| Improvement | Classification |
|---|---|
| Add CMake targets, safer validation internals, corrected formatting, bounds checks, exception override | Non-breaking source API; validation/error behavior can change |
| Add optional config/overloads for timeouts, CA roots, callbacks, `span` input; REST-only headers | Non-breaking if existing overloads retained; check overload ambiguity |
| Change silent empty/false failures to exceptions; enforce single-loop calls; adjust callback ordering | Potentially breaking behavior |
| Replace public numeric/sentinel fields with strong IDs/optional/timestamps; rename inconsistent fields | Definitely breaking source/layout |
| Remove RapidJSON constructors/public parsing; replace callbacks with coroutine interfaces | Definitely breaking source API |
| Replace uWS with an adapter while preserving public signatures | Potentially breaking timing/threading, definitely changes private object layout |
| Replace existing `std::string` constants with `string_view` directly | Potentially breaking source types/lifetimes; add typed constants first |

## 9. Networking

### Current — REST lifecycle

`kite` constructs `httplib::Client("https://api.kite.trade")` and sets `X-Kite-Version: 3`. Domain methods construct `httplib::Params` and positional format arguments. `callApi` retrieves an endpoint, calls `sendReq`, checks `http::response::operator bool`, maps API errors, and constructs DTOs. `http::request::send` directly calls blocking `Get/Post/Put/Delete`, copies status/body out of `httplib::Result`, and builds a JSON/raw response. `HEAD` exists in the enum but is unsupported by the sender and unused by endpoints.

The pinned httplib defaults are a **300-second connection timeout**, five-second read timeout, five-second write timeout, keep-alive false, redirect-following false, and certificate verification true (`httplib.h:25-42,1146-1194`). The SDK does not override these or expose a runtime timeout, proxy, custom root, CA bundle, cancellation, retry, or pool configuration. DNS uses synchronous `getaddrinfo` before socket operations; socket timeouts do not establish a full DNS-to-response deadline. Concurrent sends inside the same httplib implementation are serialized with a recursive request mutex (`httplib.h:6143`); this does not synchronize SDK credentials/configuration.

TLS hostname/certificate checks exist in pinned REST transport (`httplib.h:7818-7884`), using system/default CA paths. `SSL_VERIFY_NONE` in its handshake block does **not** prove REST validation is disabled: the implementation explicitly checks verify result, peer certificate, and hostname afterward. Actual handshake acceptance/rejection still needs integration tests.

### Finding F09 — routing/encoding defects are hidden by the mocked transport seam

- **Severity:** MEDIUM (confirmed request construction defects).
- **Location:** `kite.hpp:650-651`; `kite/order.hpp:82-96`; `kite/market.hpp:79-80`; `tests/unit/kite/market.cpp:170-181`.
- **Finding:** bracket-order cancellation formats `parent_order_id={1}` although parent ID is the third argument, so the order ID is substituted. Historical tests pass a pre-encoded `+` separator, but httplib's path encoding converts `+` to `%2B`. Other path/query arguments are interpolated without component-level encoding.
- **Why it matters:** distinct parent/child IDs yield the wrong cancellation query. Legacy historical date conventions are not exercised at the wire layer; `&`/`#` in unrestricted inputs can alter query meaning. Fixed roots prevent arbitrary-host SSRF, but not incorrect request targets.
- **Evidence:** endpoint placeholders; runtime transport helper probe turned `...from=2017-12-15+09:15:00` into `...from=2017-12-15%2B09:15:00`; quote ticker portion alone uses `parser::encodeUrl`.
- **Recommendation:** separate path/query encoding, test final request targets, and correct the legacy parent placeholder or explicitly deprecate unsupported bracket-order behavior.
- **Migration impact:** internal corrections; server-visible behavior changes. Current bracket-order availability was not established, so prioritize active APIs first.

### Finding F10 — operational transport policy is absent

- **Severity:** MEDIUM; blocking in callbacks is HIGH when users call REST there.
- **Location:** `kite/api.hpp:36-38`; `utils.hpp:480-549`; `ticker/internal.hpp:389-400`.
- **Finding:** default short read/write timeouts coexist with a five-minute connect timeout and no overall request deadline; no response-size limit/configuration is supplied by SDK. Keep-alive remains off. A REST request made in a ticker callback blocks that loop.
- **Why it matters:** failure latency and TLS connection overhead are uncontrolled; long callbacks delay market ticks/pongs. HTTP order outcomes after a dropped response are ambiguous.
- **Evidence:** transport defaults and direct synchronous dispatch; no request queue, deadline, or retries in SDK code.
- **Recommendation:** expose explicit chrono durations, deadline/cancellation policy, safe CA configuration and response limits; benchmark keep-alive. Distinguish transport failure from unknown order outcome. Do not automatically retry a non-idempotent placement after uncertain completion.
- **Migration impact:** additive configuration possible; defaults/retry behavior are potentially breaking and require release notes.

## 10. WebSocket

### Current

`ticker` creates `hub.createGroup<uWS::CLIENT>()`. `connect()` registers callbacks and starts a 3000 ms auto-ping before `hub.connect`. The URL embeds key/token query arguments; constructor connection timeout seconds are multiplied into milliseconds. `run()` blocks in `hub.run()`; no SDK thread is launched. All callbacks run directly in the uWS dispatch call. The WebSocket HTTP upgrade, TLS, frame reassembly, ping/pong, buffers, and sockets are external uWS responsibilities.

`subscribe`/`unsubscribe` serialize action + instrument list and update `subbedInstruments` after `send`; `setMode` builds JSON with RapidJSON directly and records mode. State is desired/sent state, not server acknowledgement. Invalid mode strings are transmitted unchanged but locally recorded as FULL. There is no subscription-limit check (official API permits 3000 instruments and three connections [U16]), application send completion/backpressure policy, inbound work queue, or bounded callback dispatch.

### Finding F11 — subscribe/unsubscribe send invalid JSON actions

- **Severity:** HIGH (confirmed functional defect).
- **Location:** `ticker/internal.hpp:99-129`; `utils.hpp:329-348`.
- **Finding:** `req.field("a", "subscribe")` and `"unsubscribe"` instantiate `Value` as a character array, whose decayed type is a pointer, not `std::string`. The scalar encoder handles string/integral/floating types only and leaves its buffer null.
- **Why it matters:** the server receives a null action and the SDK records subscriptions as if they were sent correctly.
- **Evidence:** offline reproduction of the same helper prints `{"a":null,"v":[408065]}`. `setMode` takes a different writer path and is not affected by this literal issue.
- **Recommendation:** implement explicit string-like encoding with constrained unsupported-type failure; add exact subscribe/unsubscribe payload tests and server-level subscription tests.
- **Migration impact:** private behavioral fix; independent of C++20 and necessary before transport replacement.

### Finding F12 — reconnect replay does not send subscribe actions

- **Severity:** HIGH for consumers relying on documented subscription semantics.
- **Location:** `ticker/internal.hpp:357-369,384`; examples `example3.cpp:32-35`, `example4.cpp:32-35`.
- **Finding:** replay only calls `setMode` for saved instruments. Initial ticker examples also set mode without calling `subscribe`.
- **Why it matters:** the official protocol specifies distinct subscribe and mode actions; the code relies on undocumented mode-only subscription behavior. After a reconnect, local subscription state is not proof that the new server connection has subscribed.
- **Evidence:** no `subscribe` call in `resubInstruments`; protocol action table and official request examples [U16]. Whether the production server currently accepts mode-only subscription was not exercised.
- **Recommendation:** replay a valid subscribe payload, then each mode group; verify ordering and wire acknowledgements/errors using a local fixture server and later controlled integration.
- **Migration impact:** server-visible correction; preserve intended modes and avoid duplicate subscription side effects.

### Finding F13 — reconnect sleeps inside event dispatch and has no cancellation contract

- **Severity:** HIGH (blocking confirmed; recursive failure risk scenario-dependent).
- **Location:** `ticker/internal.hpp:174-193,409-428`.
- **Finding:** error/disconnection callbacks invoke `reconnect`, which sleeps on the event-loop thread, doubles delay, and attempts another connect. There is no jitter or cancellation/terminal-stop flag. Persistent synchronous DNS/connect-start failure can immediately invoke the error callback again and recurse through `reconnect` until attempts are exhausted.
- **Why it matters:** timer/socket dispatch freezes during backoff; shutdown cannot interrupt the sleep. Reconnect checks `isConnected` immediately after scheduling an asynchronous connect, when connection completion has not occurred.
- **Evidence:** `sleep_for` in callback-reached code; external `Hub::connect` invokes error handler synchronously when `Node::connect` returns null [U5s]. Success resets attempt/delay in `onConnection`; terminal failure only clears `isReconnecting`.
- **Recommendation:** timer-driven explicit connection state machine with cancellable attempts, capped arithmetic, jitter, separate authentication failures, and documented normal-close behavior.
- **Migration impact:** callback ordering/timing may change; keep this separate from the standard bump.

### Finding F14 — stop does not stop the group/event loop or pending connection

- **Severity:** HIGH; confirmed wrapper omission, runtime hang depends on uWS backend.
- **Location:** `ticker/internal.hpp:81-97,430`; `ticker/ws.hpp:247-266`.
- **Finding:** `stop()` only closes `ws` if non-null. Auto-ping timer stays active, pending connects/reconnects are not cancelled, and no terminal stop state prevents later callbacks from reconnecting.
- **Why it matters:** calling stop before connection completion has no effect. With v0.14's repeating timer, closing only the socket need not let `run()` return; examples that call stop after run cannot guarantee cleanup.
- **Evidence:** upstream `Group::startAutoPing` allocates a recurring timer; `Group::close`, not `WebSocket::close`, stops that timer [U5s]. No SDK destructor/group shutdown exists.
- **Recommendation:** define idempotent terminal shutdown: cancel retries/connects, close group/timers on loop owner, drain required close callbacks, release owned group, and then permit destruction.
- **Migration impact:** potentially breaking lifecycle correction; tests must specify restart/stop behavior.

### Finding F15 — liveness and callback error boundaries are incomplete

- **Severity:** MEDIUM; callback exceptions can be HIGH in a live feed.
- **Location:** `ticker/internal.hpp:195-213,389-407`; `ticker/ws.hpp:265-266`.
- **Finding:** `lastBeatTime` updates only for one-byte binary messages *and only when `onTicks` is installed*; `lastPongTime` is recorded but never used by SDK. Malformed text and user callbacks can throw through uWS dispatch without containment. Unknown text types are ignored if nonempty; `onMessage` receives the whole envelope rather than extracted text.
- **Why it matters:** heartbeat getters mislead order-only clients. Parsing failures do not consistently reach `onError`, and a callback can unwind the networking loop.
- **Evidence:** binary branch condition `opCode == BINARY && onTicks`; direct parser/callback invocations without catch. External auto-ping handles missed pongs separately [U5s], so the SDK should not be described as having no liveness detection at all.
- **Recommendation:** update liveness independently of callback registration; choose steady-clock freshness and session reset semantics; specify/contain callback exceptions and unknown-message policy.
- **Migration impact:** mostly private fixes, potentially breaking error/callback semantics.

## 11. JSON / Data Models

### Current

RapidJSON DOM parsing reads `str.c_str()` without explicit length or schema. `http::response` owns the envelope document until DTO conversion completes. `utils::json::get` validates recognized scalar types and returns `{}` on a missing field; null strings become empty strings. Numeric nulls generally throw. Doubles accept `IsDouble` or signed `IsInt`, not all JSON numbers. Public DTOs own strings/vectors/scalars, but parsing is tightly coupled to RapidJSON.

Writers build DOM objects/arrays with allocator-owned copied strings and serialize through `StringBuffer`. Scalars support exact `std::string`, integral and floating types (bool falls into integer handling); unsupported scalar types silently yield null/moved-empty data. Array encoding accepts fundamental types or a caller-supplied encoder. `json<JsonArray>` uses the document SAX `StartArray` method rather than `SetArray`, an unusual mixed DOM/handler construction that should be characterized, not copied into a new codec.

CSV instruments pass the entire response through `stringstream` → rapidcsv DOM → per-row string vector → positional DTO conversion. Dates remain strings; WebSocket times are signed 32-bit epoch seconds; REST volume is sometimes 64-bit while ticker volume is 32-bit. Models usually initialize numerics to -1, but missing JSON fields overwrite them with zero, creating inconsistent absence semantics.

### Finding F16 — structured and binary parsers do not validate container boundaries

- **Severity:** HIGH (confirmed unchecked reads/assertion exposure).
- **Location:** `utils.hpp:119-140`; `ticker/internal.hpp:216-269`; `responses/market.hpp:198-212,236-258`; `responses/mf.hpp:239-264`; `kite/market.hpp:83-84`.
- **Finding:** extractors use `doc["data"].Get*()` without checking membership/type; catching `std::exception` cannot catch a RapidJSON assertion. Binary `unpack` creates iterator ranges and memcpy sizes without validating bounds; `splitPackets` trusts packet count/length; instrument token is read before recognizing packet size. Candle/CSV parsers index required positions without checking size. Array object elements are often accessed unchecked throughout domain methods.
- **Why it matters:** malformed/truncated/changed upstream data can abort debug processes or enter undefined behavior in release builds rather than produce a structured error. WebSocket frame validation does not validate the application packet lengths inside a valid frame.
- **Evidence:** RapidJSON `document.h:1176,1628,1809` accessors assert type; `unpack` uses `bytes.begin()+start/end` and `memcpy(sizeof(T))`; CSV requires 12/16 columns; candles require at least six fields.
- **Recommendation:** validate root/envelope/element types, packet framing and allowed sizes, counts, arithmetic, and all required row/candle widths before access; use bounded byte views and contextual parse errors; fuzz application packets and structured responses.
- **Migration impact:** private parsing fixes can preserve public models; newly rejected malformed data changes behavior intentionally. C++20 is an opportunity for span, not a substitute for bounds checking.

### Finding F17 — JSON diagnostic lambdas retain dangling stack references

- **Severity:** HIGH (ASan-confirmed memory lifetime defect).
- **Location:** `utils.hpp:145-148,219-222,245-249`.
- **Finding:** each `get` specialization initializes a static closure capturing `&name`, the local pointer parameter of its first call. Subsequent type errors format diagnostics through that expired parameter reference.
- **Why it matters:** the error path intended to protect callers can instead crash or read invalid stack memory; independent clients/threads share the specialization's closure.
- **Evidence:** an ASan probe calls `get<double>` successfully once then on a wrong-typed field. `detect_stack_use_after_return=1` reports `stack-use-after-return` for local `name` in `utils.hpp:145`, through line 147. A combined non-ASan probe also exited with SIGSEGV; it was not counted as an independent additional defect.
- **Recommendation:** make the formatter non-static or pass the field name as an ordinary argument; test repeated parsing followed by failure and run ASan.
- **Migration impact:** non-breaking implementation fix; address before wider parser changes.

### Finding F18 — valid large integer-valued numbers are rejected as doubles

- **Severity:** MEDIUM.
- **Location:** `utils.hpp:157-162,190-204`; `responses/market.hpp:201-205`.
- **Finding:** a JSON integer such as `3000000000` has neither `IsInt()` nor `IsDouble()` in RapidJSON and is rejected for double-valued money/market fields despite being a valid number.
- **Why it matters:** behavior depends on upstream numeric spelling/range rather than the target model; large balances/notionals can fail even though the value is representable in double.
- **Evidence:** offline RapidJSON probe reports `IsNumber=1 IsInt=0 IsDouble=0` for that integer; converters accept only the latter two predicates.
- **Recommendation:** accept numeric types with explicit range/precision policy; preserve exact quantities/IDs as integers and test int32/uint32/int64 boundaries, nulls, and integer/decimal spellings.
- **Migration impact:** non-breaking model API; changing field types is a separate major-version concern.

### Finding F19 — supposedly const parsing consumes input DOM arrays

- **Severity:** MEDIUM (confirmed behavior/API surprise).
- **Location:** `utils.hpp:254-265`; `responses/portfolio.hpp:159-166`; `responses/gtt.hpp:135-139`; `responses/market.hpp:125-136`; `responses/margins.hpp:175-179`.
- **Finding:** `out = it->value.GetArray()`/GetObject moves the source container into a temporary RapidJSON value. Passing a const reference to its object view does not make underlying values immutable. Collection parsers also append without clearing when reused with fresh documents.
- **Why it matters:** caller-owned DOMs are changed unexpectedly, a second parse observes emptied arrays, and public DTO `parse()` does not consistently mean replacement.
- **Evidence:** pinned RapidJSON `document.h:855-875` documents move-and-empty semantics; offline `positions(d.GetObject())` changes source `net` size from one to zero.
- **Recommendation:** traverse validated const views without ownership transfer and parse into a temporary DTO before replacing existing state. Define reparse/exception guarantees.
- **Migration impact:** behavioral correction; removal/retyping of public JSON APIs is definitely breaking.

### Finding F20 — SIP modification parses the wrong response key

- **Severity:** MEDIUM (confirmed DTO/result defect).
- **Location:** `kite/mf.hpp:108-111`; `tests/unit/kite/mf.cpp:273-298`; `tests/mock_responses/mf_sip_modify.json:3-5`.
- **Finding:** `modifyMfSip` extracts `order_id`, but the corresponding provided API fixture returns `sip_id`. Its unit test uses an order-response fixture instead of the SIP-modification fixture and therefore hides the mismatch.
- **Why it matters:** a successful modification can return an empty identifier to the caller.
- **Evidence:** exact key and mismatched test fixture in cited sources.
- **Recommendation:** use the correct contract fixture, assert the returned SIP ID, and later correct implementation/documentation semantics.
- **Migration impact:** source signature can stay string; caller-visible return value becomes correct.

### Finding F21 — data availability and completeness are not represented reliably

- **Severity:** MEDIUM.
- **Location:** `utils.hpp:213-215`; `responses/ws.hpp:50-75`; `ticker/internal.hpp:285-352`; `responses/gtt.hpp:137-150`; `tests/mock_responses/gtt_get_orders.json:74-91`.
- **Finding:** missing fields silently become zero/empty; unrecognized binary packet sizes yield mostly sentinel ticks; zero close price causes non-finite percentage change; GTT parsing discards nested execution `result`/rejection details by treating each GTT order as ordinary `order`.
- **Why it matters:** callers cannot distinguish unavailable data from actual zero values or reliably inspect triggered-order failure. `netChange` also means an absolute index price change versus percentage change for tradable quotes.
- **Evidence:** getter fallback, packet-size branches plus unconditional tick append, division by `ohlc.close`, and omitted fixture result mapping.
- **Recommendation:** define required/optional schemas and parse-mode validity; reject unknown packet sizes; guard zero denominators; preserve GTT execution results; document distinct change units. Treat stronger DTO redesign as a later versioned API change.
- **Migration impact:** adding fields is source-additive but changes layout; optional/variant/strong-time replacement is definitely breaking.

## 12. Memory and Ownership

### Current

Most REST memory is automatic/value-owned. Response documents and their allocations die after synchronous DTO conversion; copied strings/vectors survive. Credential/session strings are ordinary heap-capable strings with no secure-erasure guarantee. There is no explicit shared ownership in project source. REST responses copy httplib's body into a local string before parsing; requests copy auth/path/Params into temporary request objects.

Ticker callbacks capture the ticker object implicitly via `[&]` (effectively its `this` pointer), not a local message buffer. Tick vectors and text/postback temporaries are borrowed only for the callback duration. Applications retaining them need to copy or move into owned storage. Deleting a ticker from its own callback, or destroying it while an external thread runs its loop, has no protective lifetime protocol.

### Finding F22 — dynamically created uWS Group has no owner/release path

- **Severity:** HIGH (confirmed for inspected documented uWS source).
- **Location:** `ticker/internal.hpp:64-71`; `ticker/ws.hpp:247-251`; upstream `Hub::createGroup`, `~Hub`, `Group` [U5s].
- **Finding:** createGroup allocates with `new`; the SDK stores only a raw pointer, has no destructor, and never calls group close/delete. The inspected Hub destructor frees compressor buffers, not created groups.
- **Why it matters:** each ticker construction leaks a group/callback state on that implementation; active timers/sockets retain pointers into Hub/node/ticker storage during unsafe destruction. No explicit SDK allocation does not mean no ownership obligation.
- **Evidence:** full project ticker declarations/implementation contain no destructor/release; upstream v0.14.8 sources establish caller-owned allocation. A differently patched system uWS must be checked, not assumed identical.
- **Recommendation:** use a defined RAII group/session owner with backend-appropriate close/drain/delete ordering; explicitly disable unsupported moves/copies and test destruction in every connection state.
- **Migration impact:** private layout/lifecycle change; ABI rebuild and callback contract tests required.

### Recommendation

Use unique ownership for transport sessions when one client owns them. Introduce `shared_ptr` only when outstanding asynchronous operations genuinely extend a session lifetime, and prevent ownership cycles in callbacks. A `unique_ptr` alone does not cancel sockets/timers. F17 is a separate confirmed dangling-reference bug, not a Group issue.

## 13. Concurrency

### Current — complete SDK synchronization inventory

| Mechanism | Location | Role / guarantee |
|---|---|---|
| `hub.run()` | `ticker/internal.hpp:93` | Event loop on the caller's thread, callback dispatch inline |
| `hub.connect` | `:169-171` | External async socket/upgrade work; external DNS setup is synchronous |
| uWS auto-ping timer | `:430` | External loop callback every 3000 ms |
| `sleep_for` | `:180` | Synchronous reconnect backoff, not a worker thread |
| `atomic<bool> isReconnecting` | `ticker/ws.hpp:264` | Only reconnect flag; does not protect socket/map/timestamps/callbacks |
| HTTP request mutex | pinned httplib `:1129,6143` | Serializes request send internally, not SDK credential setters/readers |

There is no SDK thread pool, callback queue, future/promise, condition variable, strand, lock ordering, coroutine scheduler, or explicit mutex. Ordinary single-threaded examples use one ticker loop and inline callbacks. Callback reentrancy is possible: `onConnect` subscribes/sets mode; `onTicks` calls stop; disconnect callbacks run application code before retry.

### Finding F23 — cross-thread use is unsafe and undocumented

- **Severity:** HIGH when users run the ticker in a worker and control it elsewhere; MEDIUM documentation priority alone.
- **Location:** `ticker/ws.hpp:69-112,246-266`; `ticker/internal.hpp:73-97,105-109,419`; `kite/api.hpp:40-51`.
- **Finding:** `ws`, subscription map, key/token, chrono timestamps, and callback functions have unsynchronized reads/writes. Credential mutation during REST request construction also races despite httplib's send mutex.
- **Why it matters:** a UI/control thread calling `stop`, `subscribe`, `isConnected`, or callback setters while another thread dispatches can race with disconnect/socket destruction. Atomic reconnect state does not establish safe compound state access.
- **Evidence:** no locks/queue around cited operations; external legacy optional `UWS_THREADSAFE` cannot protect SDK containers or callback fields.
- **Recommendation:** document single-owner-loop affinity immediately; later marshal mutating operations through a loop/executor queue, publish safe snapshots, and prohibit concurrent destruction. Test reentrancy and cross-thread shutdown with TSAN where supported.
- **Migration impact:** clarifying a contract is additive; introducing executor delivery changes timing. `jthread` does not fix these races by itself.

No deterministic SDK mutex deadlock was found. Blocking backoff/callback work and backend-dependent shutdown hangs are supported findings; inventing a lock-order deadlock would not be justified.

## 14. Error Handling

### Current

- Transport failures become `libException` containing a stringified httplib error.
- API non-success responses generally map `error_type` to named `kiteppException` subclasses carrying HTTP code/message.
- Unknown/absent error names can become generic library failures, losing HTTP context.
- `getInstruments`/`getMfInstruments` return empty on non-200 raw response; `invalidateSession` returns the response's boolean status, bypassing normal API exception mapping.
- Numeric CSV conversion can throw `std::invalid_argument`/`std::out_of_range`; fmt can throw its own format exception; RapidJSON misuse asserts. These are not unified SDK errors.
- Ticker uses connection/error/close callbacks but also throws during parsing/subscription; no containment adapter exists.
- No public `std::error_code`, expected/result object, structured network cause, or logging-only fallback exists. Missing JSON fields often silently return default values.

### Finding F24 — libException does not override std::exception::what

- **Severity:** MEDIUM (confirmed error-reporting defect).
- **Location:** `exceptions.hpp:205-219`.
- **Finding:** `const char* what()` is non-const and lacks `noexcept`, so it is a different function from the virtual base `what() const noexcept`.
- **Why it matters:** `catch (const std::exception&)` prints the base diagnostic rather than the library's useful message; `catch (const libException&)` cannot call the intended overload.
- **Evidence:** offline probe through `const std::exception&` prints `std::exception` instead of the synthetic detail.
- **Recommendation:** correct signature and add `override`; test base and derived catches.
- **Migration impact:** source-compatible correction for intended use; adding the intended virtual override changes behavior and requires consistent header rebuilds.

### Finding F25 — response failure semantics depend inconsistently on HTTP/envelope shape

- **Severity:** MEDIUM (confirmed internal logic; production HTTP-200 error scenario not asserted).
- **Location:** `utils.hpp:458-473`; `kite/market.hpp:99-100`; `kite/mf.hpp:135-137`; `kite/api.hpp:66-69`; `exceptions.hpp:239-267`.
- **Finding:** JSON status/error fields are examined only when HTTP status is not 200; a 200 error envelope is treated as success. Missing error_type overwrites default `NoException` with empty string, then loses HTTP context. CSV failure is indistinguishable from a valid empty collection. Malformed/non-JSON proxy errors fail JSON parsing before status classification.
- **Why it matters:** callers cannot make consistent recovery decisions or distinguish empty market data, authorization failure, upstream outages, and schema failure.
- **Evidence:** runtime synthetic HTTP-200 error probe prints accepted=true; direct branches/early returns. No claim is made that Zerodha routinely emits HTTP-200 error envelopes.
- **Recommendation:** classify transport, HTTP, envelope, schema, and user-callback failure separately; preserve status/cause/retryability, retain compatibility exceptions at public boundaries, and document special bool-return operations.
- **Migration impact:** changing existing empty/false failure behavior is potentially breaking. A result-returning API should be additive/versioned, not a blanket expected rewrite.

**Future:** a small structured error record with category, HTTP status, transport cause, safe message and endpoint context can support both exceptions and optional `try_*` results. On a strict C++20 baseline, use a small local result/variant only if justified, or an explicitly chosen expected backport. `std::expected` requires C++23 and is not required for modernization.

## 15. Security

### Current

REST auth and WebSocket auth reside in separate ordinary strings. Session generation computes the official SHA-256 concatenation checksum; ordinary requests use `Authorization: token key:token`, not per-request HMAC. The SDK neither runs a login server nor verifies external webhook signatures. A `postback.checksum` field is parsed, not independently authenticated. TLS-authenticated transport must protect streaming identity.

No credential files or persistent storage are supplied. Tests contain credential-shaped synthetic constants and fixtures; values are intentionally not reproduced here. No live validity was tested, and their use in tests is not proof of deployed credentials. Runtime examples obtain secrets from environment variables without a storage service. The library does not log automatically, but its error strings/callbacks can expose sensitive payloads.

### Finding F26 — WSS encryption lacks peer authentication in documented legacy transport

- **Severity:** BLOCKER for a secure modernized release; HIGH security impact today.
- **Location:** `ticker::connectInternal`, `ticker/internal.hpp:169-171`; documented hoytech uWS v0.14.8 and current master `src/Node.cpp`/`Node.h` [U5s].
- **Finding:** SDK uses default `Hub::connect` and has no TLS verification settings. In inspected uWS sources, the client context is created and SSLv3 disabled, but no peer verification mode, trust roots, certificate result, or hostname check is configured. SNI alone is not certificate authentication.
- **Why it matters:** an attacker with network/DNS/proxy interception can impersonate the streaming endpoint, observe the token in the upgrade query, and supply fraudulent ticks/order updates over an encrypted but unauthenticated connection.
- **Evidence:** official fork source's `Node::Node` and secure `Node::connect` blocks; no SDK corrective hook. REST has separate explicit validation and must not be conflated with this WSS finding.
- **Recommendation:** use a maintained client transport configured with trust roots, verify-peer and hostname verification; test wrong-host, self-signed, expired/untrusted chains, and trusted certificates. If preserving a locally patched uWS, verify the actual installed revision/configuration first.
- **Migration impact:** security-critical transport/configuration work; backend replacement need not change public signatures but changes timing and private layout. No live MITM test was performed.

### Finding F27 — examples and parse errors disclose secrets/personal data

- **Severity:** HIGH when stdout/errors are collected centrally; MEDIUM for examples alone.
- **Location:** `examples/example1.cpp:34-47`; `README.md:116-129`; `docs/mainpage.md:85-98`; `utils.hpp:302-306`.
- **Finding:** example prints an access token; malformed JSON exception embeds the entire original body, potentially including session tokens/account data. Environment results are used to construct strings without checking null.
- **Why it matters:** users copy examples into systems with logs; missing environment variables cause invalid null-string construction rather than an actionable configuration error.
- **Evidence:** print/read/exception statements at cited locations; no credential values are included in this report.
- **Recommendation:** validate environment variables, redact tokens, and report parse offset/category with bounded safe context rather than response content.
- **Migration impact:** non-breaking SDK implementation improvement; example output/error text changes.

Additional confirmed security surfaces are application packet/JSON/CSV validation (F16), error-path lifetime (F17), unverified lifecycle (F22), and stale bootstrap (F04). URL queries necessarily carry WSS credentials under the official protocol; do not log those URLs. TLS/dependency version checks should distinguish a supported distributor backport from an obsolete raw upstream archive. A future TLS change should not add certificate pinning or a new crypto provider without a concrete deployment requirement.

## 16. Testing

### Current — inspected test system

There are **42 TEST cases**: 41 REST cases across eight domain files and one ticker binary parsing case. Framework is external GTest/GMock. `mockKite` overrides `sendReq` through `KITE_UNIT_TEST`; tests compare endpoint templates, Params, format arguments, and happy-path parsed DTO values. They do not execute production HTTP transmission, final URL encoding, TLS, DNS, timeouts, or concurrency. `endpoint::operator==` does not compare response type (`utils.hpp:416-418`).

The ticker test loads two full 184-byte instrument packets from one binary fixture; no actual WebSocket is connected. It does not cover LTP/quote/index/currency/BSECDS, heartbeat dispatch, malformed/truncated input, unknown packets, reconnect, subscription payloads, stop/destruction, or TLS. No meaningful negative/error/concurrency tests, mocks for the streaming backend, fuzz targets, integration servers, sanitizer runs, or coverage reporting exist in root CI. `tests/unit/main.cpp` is not in the REST test source glob; library-provided main is linked instead.

### Finding F28 — baseline tests are insufficient and location-dependent

- **Severity:** BLOCKER for safe migration; HIGH test-infrastructure debt.
- **Location:** `CMakeLists.txt:75-95`; `tests/unit/utils.hpp:56-74`; REST fixture reads; `tests/unit/tickertest.cpp:40`.
- **Finding:** fixture paths use `../tests/...` relative to runtime cwd, assuming the build directory is a direct child of repository root. CTest supplies no alternative fixture root. Genuine external build trees resolve fixtures incorrectly. The mock seam bypasses code where multiple confirmed defects live.
- **Why it matters:** historical green tests cannot establish transport parity, parser safety, or lifecycle behavior; installing a newer compiler/library may change failures without a diagnostic baseline.
- **Evidence:** every unit source read, CTest registrations, 42-count search; subscription malformed JSON and SIP fixture mismatch escaped existing suite.
- **Recommendation:** first make fixture resolution independent of cwd, restore exact baseline builds, and add the acceptance tests below before migration. Keep GTest/GMock.
- **Migration impact:** test/build-only changes initially; introduction of stable injection seams may change private layout.

### Recommendation — highest-value tests, in order

1. **Compile contract:** supported umbrella, REST/ticker split headers, standalone headers, two-TU consumers, installation/add_subdirectory, uniform macro configuration; GCC/Clang C++17 baseline and C++20 lane.
2. **Parser safety:** repeated getters then wrong type (F17); malformed root/data/array elements; null/missing required/optional fields; large numeric boundaries; short CSV/candles; parse reuse and DOM immutability; ASan/UBSan and bounded packet fuzzing.
3. **Subscription protocol:** valid subscribe/unsubscribe JSON, modes/limits, replay subscribe-before-mode, disconnect during writes, invalid modes; no silent state success after a failed send.
4. **Lifecycle:** connect failure/success, pending-connect stop, normal/abnormal close, capped retry and auth failure, cancellable backoff, stop from callback, repeated connect, group/timer release, destruction with outstanding operations.
5. **HTTP wire contract:** local server verifies headers, URL encoding, Params/JSON bodies, final parent cancellation argument, SIP-modify response, 401/403/429/5xx/non-JSON failures, timeout/cancellation and ambiguous order outcome.
6. **TLS:** trusted/untrusted/wrong-host/expired certificate tests for both stacks with synthetic tokens and a local server.
7. **Feed coverage:** all allowed packet sizes, index/currency divisors, zero close, heartbeat without onTicks, callback throw, full-depth parsing, packet length/count truncation.
8. **Concurrency:** owner-affinity assertions, posted commands, connection snapshots, callbacks during close, cross-thread stop if supported; TSAN on suitable platforms.

**Finding:** the project is not safe to migrate broadly without first adding these tests. A small compile-only C++20 experiment is useful, but not evidence that dependency/transport/API replacement preserves trading behavior.

### Current — checks performed during audit

| Check | Result |
|---|---|
| Git status/diffs/submodules at start | Clean tracked/staged state; eight initialized pins |
| GCC 13.3, Linux/aarch64, local OpenSSL/zlib | Available; pkg-config reports OpenSSL 3.0.13, zlib 1.3 (host observations, not recommended upstream baselines) |
| C++17 REST implementation syntax | Pass with `-DCPPHTTPLIB_OPENSSL_SUPPORT -I include -Wall -Wextra -Wpedantic -fsyntax-only`; warnings for Clang pragmas, macro extra semicolons, missing request aggregate initializer |
| Same C++20 syntax | Fail at `kite::loginURL` runtime fmt string (F05) |
| Supported umbrella syntax | Fail due missing `uWS/uWS.h`, not a new project compilation defect |
| Offline subscription writer probe | Confirmed null action (F11) |
| ASan getter probe | Confirmed stack-use-after-return (F17), expected diagnostic failure |
| Offline response/exception/DTO probe | Confirmed HTTP-200 error accepted, base what loses message, DOM net array consumed |
| Offline numeric/URL helper probe | Confirmed large integer predicate mismatch, plus encoded as `%2B`, and incorrect UTF-8 percent encoding under `-fsigned-char`; no live HTTP call |
| Root CMake/CTest, Clang/MSVC, live network | Not run: tooling/development dependencies unavailable; no production orders/credentials used |

Probes were compiled from stdin; executables were isolated under `/tmp/omnirush/`, outside the repository. They are audit diagnostics, not additions to the project test suite. The downloader, CI workflow, Docker build, formatter, package installation, submodule update and branch checkout were not run.

## 17. Performance

### Finding F29 — avoidable allocation/copy cost in tick parsing

- **Severity:** MEDIUM performance debt, **Confirmed cost**, unmeasured impact.
- **Location:** `ticker/internal.hpp:216-228,262-263,334-352`.
- **Finding:** each incoming frame is copied into a vector, each packet is copied into its own vector, and every integer field allocates/copies/reverses a temporary vector before memcpy. Full ticks then copy `Tick` including two depth vectors into the output collection.
- **Why it matters:** per-field heap allocation is unnecessary on a high-frequency feed and adds allocator churn/latency variance.
- **Evidence:** vector constructions in unpack/split/parse, `ticks.emplace_back(Tick)` with an lvalue; no buffer/packet/tick reserve based on known counts.
- **Recommendation:** first correct validation, then bounded byte views/direct big-endian integer loads, reserve tick count, move results, and evaluate fixed depth arrays behind compatibility adapters. Benchmark representative frames before/after.
- **Migration impact:** private optimization non-breaking; replacing public depth vectors by arrays changes API/layout.

| Assessment | Other issue / opportunity | Evidence and response |
|---|---|---|
| Confirmed cost | Every client constructs a string-keyed endpoint unordered_map | `kite.hpp:634-706`; share immutable descriptors only after measuring client creation |
| Confirmed cost | Copies HTTP body before DOM parse, raw CSV → stream → CSV cells → DTO strings | `utils.hpp:489-540,587-594`; consider move/streaming boundaries after transport tests |
| Confirmed behavior | Keep-alive off; callback dispatch and backoff block loop | F10/F13; measure TLS handshake and tail latency, fix loop blocking first |
| Likely issue | Large instrument dumps have several simultaneously live representations | Raw string, stream/rapidcsv rows, output; no memory measurements collected |
| Possible optimization | Reserve model vectors/maps; simplify runtime string formatting and type-erased custom parser lambdas | Domain collection loops / `CustomParser`; do not complicate APIs without profiles |
| Possible optimization | `std::from_chars` for explicit locale-free CSV numeric conversion | Existing `stoi/stod` accept trailing junk, depend on locale and throw; validate exact consumption first |
| Low priority | Namespace `const std::string` constants initialize per TU | `userconstants.hpp:41-102`; inline constexpr typed views can reduce duplication, but direct type replacement can break callers |

No project benchmarks or data support claims of throughput, lock contention, N+1 network requests, or quadratic behavior. No logging dependency or lock-free queue should be introduced as a speculative optimization.

## 18. Documentation

### Finding F30 — documentation fails to establish supported build and runtime contracts

- **Severity:** MEDIUM.
- **Location:** `README.md:23-64,95-101,155-190`; `docs/mainpage.md`; `examples/example2.cpp`; `docs/header.html:1`; CI workflow.
- **Finding:** clone instructions omit `git` before `submodule update`; dependency list omits bundled components/link requirements; downloader output location is misstated; unsupported compiler/architecture minima are unspecified. Docs commands need `BUILD_DOCS` but example configure omits it. No timeout, TLS distinction, affinity, borrowed callback lifetime, cancellation/backpressure, error consistency, or token-expiry guidance is provided. `example2` compiles mainly commented snippets, so CI does not validate those API examples.
- **Why it matters:** users can follow instructions yet fail to build/run, or use callbacks across threads unsafely. Commented examples include broken wrapped strings and SIP creation without Installments, and cannot serve as compiled contract tests.
- **Evidence:** direct README/docs/example inspection; `example2.cpp:350-355` omits required installments count; identical README/mainpage maintenance burden; custom HTML dates to Doxygen 1.9.1.
- **Recommendation:** later consolidate duplicated narrative, document tested compilers/OS/architectures, complete link/install commands, define ownership/lifecycle/error/TLS policy, and compile executable example snippets independently. Remove token printing (F27).
- **Migration impact:** documentation changes non-breaking; publicly declaring a narrower support matrix can exclude untested consumers.

**Current supported-platform evidence:** CI intends Ubuntu and macOS, Windows entry is a TODO. README gives Linux and generic others; no precise compiler minimum, architecture list, or Windows support guarantee is declared. SDK endian handling relies on platform macros/`endian.h` or `sys/endian.h`, and the external backend controls much portability. This audit verifies only Linux/aarch64 REST syntax. A proposed GCC 13+, Clang 17+, Apple Clang with tested libc++, and VS2022 policy is a **recommendation**, not a discovery of existing support.

### Finding F31 — URL encoder depends on platform char signedness

- **Severity:** MEDIUM when non-ASCII symbols are supplied; LOW for ASCII-only valid symbol sets.
- **Location:** `include/uri-parser/include/parser.hpp:33-42`; `kite/internal.hpp:48-52`.
- **Finding:** the percent encoder iterates `char` and shifts/divides it before converting to unsigned byte values. On signed-char builds, bytes above 127 produce incorrect hexadecimal output.
- **Why it matters:** the same UTF-8 input builds a different, invalid query component across compiler/platform defaults. This is a concrete portability defect in a public quote-input path, even if current instruments are mostly ASCII.
- **Evidence:** encoding UTF-8 bytes C3 A9 gave `%C3%A9` under this host's default char mode and `%3*%10` with GCC `-fsigned-char`. No claim is made that this synthetic value identifies an actual tradable instrument.
- **Recommendation:** perform percent encoding on unsigned bytes in the selected transport URL boundary and add byte/UTF-8 wire goldens; avoid reaching into undocumented transport detail namespaces as a public dependency contract.
- **Migration impact:** internal/dependency correction, non-breaking signature; byte-correct wire behavior changes only affected inputs.

## 19. C++20 Readiness

### Current — effort assessment

- **Language-only migration:** small-to-moderate after F05, a real target-based standard contract, and compiler matrix tests. Core code is already C++17; no broad deleted-language-feature use was found in SDK source.
- **Safe modern dependency migration:** moderate-to-high, driven by external uWS, TLS/lifecycle defects, public JSON coupling, and missing negative/integration tests.
- **API redesign/async rewrite:** high and optional; do not bundle it with a standard change.

| Classification | Required work |
|---|---|
| BLOCKER | F05 runtime fmt calls; F02 CMake standard support; F28 tested baseline; verified dependency versions/ABI; F26 secure streaming release contract |
| HIGH | F11 subscription serialization, F16/F17 parser safety, F22 RAII cleanup, F13/F14 lifecycle, F08 auth coherence, F03/F04 reproducibility, F23 affinity |
| MEDIUM | Unified errors/config, wire-contract defects, DOM immutability, numeric/schema policy, DTO/codec separation, portable endian/time and unsigned-byte encoding (F31) |
| LOW | Constant/string naming cleanup, theme metadata, macro reduction, modules exploration only if measured build benefits |

C++20 does not itself repair race conditions, ownership, trust roots, schema validation, or C ABI mismatches. Validate compiler **and standard library** support for selected facilities; a `-std=c++20` flag alone does not promise full `<format>`/chrono/coroutine library support. Retaining fmt avoids making std::format availability a new platform blocker.

## 20. Migration Risks

| Risk | Current boundary | Required mitigation |
|---|---|---|
| ABI/layout | Header-only clients embed transports/DOM-dependent structs | Rebuild all consumers; no promise that mixing dependency versions or SDK headers is binary-safe |
| ODR/macros | OpenSSL/FMT macros, test-only virtual class definition | Uniform build configuration; two-TU tests; remove test macro dependence later |
| Wire compatibility | Form Params, precision/string conventions, runtime endpoint formatting | Golden final-wire tests before changing serializers/transports |
| Error compatibility | Throws vs empty/false; std::exception what | Explicit old/new policy; additive APIs/deprecations for broad error changes |
| Callback compatibility | Inline uWS loop delivery and borrowed temporaries | Define owner/executor, ordering, exception and lifetime contracts before replacement |
| Shutdown/reconnect | Legacy group/timer ownership and implicit state | State-machine tests; safe cancellation and destruction independently |
| JSON compatibility | Public RapidJSON types and move-consuming parsers | Introduce codec boundary and compatibility adapter before JSON replacement |
| Dependency versions | Unpinned external libraries, modern httplib OpenSSL/64-bit policy | Lock/test supported versions; state a 64-bit target policy if adopting current httplib |
| OS/toolchain | Windows untested; macOS dependency paths stale | CI matrices, supported compiler/libc++ floors, package config targets |
| Feature drift | Pinned fixtures/DTO fields predate upstream additions | Contract review against official API; add fields/features separately, not automatically during standard change |

## 21. Recommended Modern Dependencies

Recommendations use the official snapshot in §6. Newest release is evidence of health, not a command to deploy it untested.

### 21.1 Build tooling

```text
Current: CMake >=3.10, global configuration, no SDK targets/package.
Reason for change: C++20 standard support and reproducible consumer requirements.
Candidate: Target-based CMake with a tested 3.24+ minimum; current 4.4.x CI lane.
Why: Standard features, imported deps, install/export tests, presets and clean builds.
Alternative: A higher tested CMake floor if future modules truly require it.
Migration complexity: Low–medium; mostly build/packaging.
Risk: Older hosts lose support; export paths/transitive dependencies must be validated.
Recommendation: Retain CMake; modernize targets and configuration before language migration.
```

Choose one optional dependency-management approach only if reproducibility cannot be achieved with package-configured system libraries plus immutable submodules. FetchContent can be sufficient for test tooling; vcpkg/Conan should not both become mandatory. Do not fetch packages implicitly into the source tree.

### 21.2 HTTP

```text
Current: cpp-httplib 0.11.4 blocking HTTPS.
Reason for change: Old fixes/configuration; missing SDK timeout/cancellation/test seam.
Candidate: Supported current cpp-httplib release (observed 0.59.0), upgraded in place.
Why: Fits synchronous REST API, small header-only integration, actively maintained.
Alternative: Beast HTTP only if one executor-based async transport is an explicit requirement.
Migration complexity: Low–medium for in-place upgrade; high for async replacement.
Risk: New OpenSSL >=3 requirement, explicit 64-bit support policy, changed TLS/error behavior.
Recommendation: Keep cpp-httplib for REST initially. Isolate transport and test wire/TLS behavior.
```

Do not add libcurl, CPR, and Beast HTTP simultaneously. HTTP/2/3 is not required by the inspected API and does not justify another stack.

### 21.3 WebSocket and async networking

```text
Current: External uWS v0.14 Hub/Group client, undocumented ownership/configuration.
Reason for change: Peer authentication, cancellable nonblocking retries, lifecycle and support burden.
Candidate: Boost.Beast WebSocket + Boost.Asio + OpenSSL; observed Boost release 1.92.0.
Why: Official portable client/TLS examples, explicit async timers/executors/cancellation, mature
     WebSocket implementation. It can preserve one-owner-loop callbacks behind an adapter.
Alternative: Current cpp-httplib WebSocketClient, now documented upstream, for a deliberately
             blocking/few-connection implementation; avoids adding Boost.
Migration complexity: High for Beast adapter/state machine; medium for blocking httplib adapter.
Risk: Beast adds Boost template/build weight and still needs correct verify-peer/hostname setup,
      write serialization, lifetime ownership and bounded queues. httplib WS adds heartbeat-thread
      and blocking-read semantics; its newer WS component needs independent maturity/latency tests.
Recommendation: Beast/Asio is the primary candidate if preserving an asynchronous event loop.
                First compare a small adapter prototype against current httplib WS under the same
                lifecycle/TLS/replay/latency tests; choose the simpler passing architecture.
```

This decision is justified by actual client security/lifecycle problems, not a claim that hoytech has no maintainers. Modern mainline uWebSockets is a server-oriented different API and must not be proposed as a drop-in version bump. Beast streams are not intrinsically thread-safe; use owner/executor serialization [U17]. If Asio is selected, remove uWS and its libuv requirement together. Coroutines are optional implementation detail after callback correctness, not the initial rewrite objective.

### 21.4 TLS

```text
Current: Externally found OpenSSL, obsolete optional 1.1.1i archive.
Reason for change: Upstream support lifecycle, secure/consistent configuration of both clients.
Candidate: Supported OpenSSL 3.5 LTS (3.5.9 observed), with package-provided patches.
Why: Supported until April 2030; already required; avoids unnecessary crypto-provider transition.
Alternative: Supported distro OpenSSL build with demonstrable security support/backports.
Migration complexity: Medium across legacy uWS/httplib builds; lower after verified adapters.
Risk: 1.1.1 to 3.x ABI/provider/deprecation changes, CA discovery/platform differences.
Recommendation: Keep OpenSSL; update supported policy and verification tests. Avoid beta 4.1 and
                unnecessary 4.x-major churn when long-term support is the objective.
```

### 21.5 JSON

```text
Current: RapidJSON post-1.1.0 snapshot exposed in public DTO parsing.
Reason for change: Wrapper safety/ownership issues and slow tagged-release cadence.
Candidate: Retain and isolate RapidJSON first; later nlohmann/json 3.12.0 if maintainability wins.
Why: Immediate correctness fixes do not require replacing an entire public parser ecosystem.
     nlohmann/json supplies maintained value-oriented parsing/conversion and clear exceptions.
Alternative: Boost.JSON if Boost is already chosen and benchmark/allocation needs justify it.
Migration complexity: Low–medium for wrapper hardening; high for replacing public signatures.
Risk: DOM allocation/performance, numeric/null differences, source breaks, changed exceptions.
Recommendation: Do not replace JSON during the C++20 standard bump. Establish codec boundary,
                schema/golden tests and measured workload before selecting exactly one library.
```

SIMD parsing adds complexity without a measured JSON bottleneck; WebSocket tick hot paths are binary, so a faster JSON parser is not the first performance remedy.

### 21.6 Formatting

```text
Current: fmt 9.1.0, FMT macro/header-only, named arguments.
Reason for change: Current supported fixes and C++20 integration hygiene.
Candidate: Retain fmt, refresh to a tested 12.2.0-era release.
Why: Mature, portable, compile-time checks; existing named/runtime formatting is supported.
Alternative: std::format after standard-library support and named-argument redesign are proven.
Migration complexity: Low–medium, including explicit runtime strings and include changes.
Risk: Header/API changes between majors, ODR configuration, compiler floor differences.
Recommendation: Keep fmt; fix F05 first. No std::format switch solely to remove a dependency.
```

### 21.7 Tests

```text
Current: External GoogleTest/GoogleMock, old finder/pin, narrow mocked happy-path tests.
Reason for change: Reproducible modern build and critical coverage gaps.
Candidate: Current GoogleTest/GoogleMock (v1.18.0 observed), official config targets.
Why: Existing test style and mocks remain useful; no framework rewrite cost.
Alternative: Supported distro package resolved by config, or opt-in immutable FetchContent pin.
Migration complexity: Low for framework update; medium–high for missing behavioral infrastructure.
Risk: Compiler/support policy and test dependency packaging; not a runtime SDK dependency.
Recommendation: Retain framework; spend effort on transport/lifecycle/negative tests and sanitizers.
```

### 21.8 CSV, URL, hashing, compression, logging and docs

```text
Current: rapidcsv 8.69; tiny uri-parser; PicoSHA2; zlib/libuv via uWS; no logger.
Reason for change: Validated rows/encoding, dependency count after transport decision.
Candidate: Keep/refresh rapidcsv; transport URL encoder (or Boost.URL if already selected);
           optionally OpenSSL EVP SHA-256; remove unused uWS transitive dependencies.
Why: Reuse maintained code already present and reduce, rather than multiply, components.
Alternative: Keep PicoSHA2 while checksum tests pass; small audited local percent encoder.
Migration complexity: Low–medium; URL wire conventions and checksum goldens are essential.
Risk: Encoding/double-encoding, CSV numeric/locale differences, hash hex output mismatch.
Recommendation: No new CSV/logger library. Keep zlib if actually required; keep libuv only for
                a selected backend. Doxygen/theme can be refreshed together after core work.
```

**Dependencies that should not be replaced now:** CMake, cpp-httplib REST, fmt, rapidcsv, OpenSSL, GTest/GMock, and Zerodha fixtures. RapidJSON should first be isolated rather than automatically replaced. PicoSHA2 can remain because its narrow stable use compiles and has a golden session test. There is no requirement for spdlog, CLI libraries, a DI framework, modules tooling, or a generic async runtime beyond the chosen transport.

## 22. Recommended Refactoring

### Recommendation — small, reviewable boundaries

1. Correct F17/F11/F08/F20/F24 with targeted regression evidence on the C++17 baseline; avoid broad stylistic edits.
2. Establish explicit request/response transport seams that tests can exercise without changing class definitions through macros. Preserve public synchronous REST methods.
3. Separate HTTP, JSON codec, CSV codec and binary packet decoder from `utils.hpp`; make codecs consume validated const views and return owning models.
4. Extract ticker lifecycle into one-owner state machine, with independent desired subscriptions, actual connection generation, retry timer, cancellation and close/drain logic.
5. Adopt tested security/transport configuration; never retain full auth response bodies in error text.
6. Introduce additive typed options/IDs/results and deprecate old string/sentinel APIs only in a planned versioned cycle.
7. Optimize binary allocation paths after safety gates and benchmarks. Replacing fluent macros/naming style is low priority.

**Future:** public DTOs and synchronous REST facade can remain stable while private codec/transport adapters evolve. A ticker-owned session/executor can manage sockets/timers and publish callbacks according to a written affinity/borrowing contract. The detailed future boundaries in architecture.md are recommendations, not classes currently implemented.

## 23. Prioritized Technical Debt

| Order | Severity | Finding IDs | Concrete deliverable / exit evidence |
|---|---|---|---|
| 1 | BLOCKER | F28, F04 | Reproducible exact C++17 compile/test baseline; cwd-independent fixtures; consumer smoke build |
| 2 | HIGH | F17, F16, F11 | ASan-safe JSON errors; bounded packet/structured reads; valid subscribe/unsubscribe wire tests |
| 3 | BLOCKER/HIGH | F26, F22, F14, F13, F12 | Verified WSS identity; terminal stop/destruction/retry/replay acceptance suite |
| 4 | HIGH/MEDIUM | F08, F07, F20, F24, F25, F09 | Coherent auth, validated required parameters, correct IDs/URLs and preserved errors |
| 5 | BLOCKER/HIGH | F02, F01, F03, F05 | Modern exported targets and compile matrix; explicit runtime fmt integration |
| 6 | HIGH/MEDIUM | F23, F06, F19, F18, F21, F31 | Written affinity/API boundaries, non-consuming codecs, numeric/absence policy and portable byte encoding |
| 7 | MEDIUM | F10, F29 | Explicit network policy; measured allocation/latency benchmark |
| 8 | MEDIUM/LOW | F30, F27, F00 | Accurate compiled examples/docs, redacted output, cleaned theme metadata |

Severity describes release priority, not proof that every defect requires C++20-specific changes. Runtime safety work remains valuable even if the language migration is postponed.

## 24. Migration Plan

### Phase 0 — Audit (completed)

Deliver this report and source-derived architecture at the exact baseline. Capture pins and diagnostic results; preserve all source/build/dependency state.

### Phase 1 — Reproducible C++17 baseline and protection tests

**Work:** lock external uWS baseline and system dependency policy; repair fixture location and build/test discovery; compile actual supported umbrella and sample consumers; add parser/error/subscription/auth/wire regression tests. Add Linux GCC/Clang, macOS, and selected Windows/64-bit lanes before claiming support.

**Exit:** baseline test suite runs offline from both repository-child and external build trees; public examples/contracts compile; ASan/UBSan expose known parser issues with stable reproducers; full logs distinguish missing environment dependencies from code failures.

### Phase 2 — Safety, security and lifecycle stabilization

**Work:** fix confirmed dangling capture and subscription serialization, validate external data, correct auth/SIP/error/request defects. Decide secure WSS trust policy and implement/test terminal shutdown, cancellable backoff, subscription replay and callback exception/affinity contract. If the old backend cannot satisfy that contract safely, move its replacement forward into this phase.

**Exit:** malformed input never performs unchecked reads/assertions; subscription actions are valid; trusted/untrusted TLS tests pass; stop works during connecting/backoff/connected/disconnected and no groups/timers leak. No uncertain-order automatic retry is introduced.

### Phase 3 — Build-system and dependency boundary modernization

**Work:** namespaced target-based CMake, minimum/version policy, REST/ticker/model separation, optional tests/docs/examples, install/export package smoke test, imported networking/test targets, uniform macro/configuration, updated CI/container bootstrap.

**Exit:** default consumer builds actually validate SDK headers; exported package transitive links work; CMake minimum/current lanes agree; build uses immutable dependency identities and never writes downloads into source.

### Phase 4 — Controlled dependency updates

**Work:** refresh OpenSSL support policy first, then cpp-httplib, fmt, rapidcsv and GTest independently against golden tests. Evaluate Beast/Asio vs current httplib WS adapter if not already required by phase 2. Keep RapidJSON initially; avoid simultaneously changing schema/JSON/transport/standard.

**Exit:** each dependency change has isolated passing baseline/wire/TLS/lifecycle tests; supported 64-bit architecture/compiler policy is written; removed backend deps no longer link accidentally; changes to callbacks/errors are explicit.

### Phase 5 — C++20 language migration

**Work:** fix all runtime fmt integration; raise per-target standard requirement to C++20; compile without extensions; apply minimal span/endian/concept/chrono changes only where they improve verified code. Test oldest supported compiler/stdlib and current compilers; keep modules/coroutines optional.

**Exit:** supported public headers, installed consumer, examples, tests and sanitizers pass under C++20 on declared platforms. Wire outputs/models/callback semantics match phase 4 goldens. Update minimum compiler/toolchain release policy and require downstream rebuilds.

### Phase 6 — Versioned API/model modernization

**Work:** typed IDs/modes/request validation, explicit optional/variant data states, consistent timestamps/quantities/error result overloads, removal of public parser coupling through compatibility/deprecation adapters.

**Exit:** source/API diff reviewed; definitely breaking changes are grouped in a major release; migration examples demonstrate old/new usage and all error paths.

### Phase 7 — Measured performance, documentation and release

**Work:** bounded zero-copy packet decoder, reserves/moves, optional keep-alive, representative throughput/latency/allocation measurements; compile all documented examples; refresh Doxygen/theme and remove duplicate/stale instructions; release notes, package/version/ABI rebuild guidance.

**Exit:** benchmark improvements do not regress safety/ordering; docs name supported compilers/OS/architectures, trust roots, error and thread/lifetime contracts; a clean packaged consumer builds from scratch.

## 25. Final Recommendation

**Recommendation:** authorize a stabilization-first migration, not a simultaneous C++20/JSON/network/API rewrite. The first implementation step is a reproducible C++17 baseline plus regression tests for the confirmed parser-lifetime, subscription, auth and wire-contract defects. Address streaming peer authentication and shutdown before calling any modernized release production-ready. Modernize build targets, update justified dependencies one at a time, then enable C++20 with the known fmt correction and a compiler/stdlib matrix.

**Current audit verification:** both audit documents are provided; no existing source/header, CMake/build, dependency pin, submodule checkout, or Git history was changed. The architecture describes actual implementation, while future proposals are labeled separately. Full CTest/TLS/runtime transport certification remains an explicit next-phase acceptance gate because local required tooling was unavailable.

### Official upstream evidence register

All URLs below were inspected on 2026-10-05. Latest-release links are time-sensitive; lock immutable revisions during implementation rather than relying on them at build time.

- **[U1] CMake:** [downloads](https://cmake.org/download/), [CXX_STANDARD](https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html), [find_library](https://cmake.org/cmake/help/latest/command/find_library.html). Observed 4.4.4 / legacy 3.31.12; standard 20 added 3.12.
- **[U2] cpp-httplib:** [release v0.59.0 via latest](https://github.com/yhirose/cpp-httplib/releases/latest), [official README](https://github.com/yhirose/cpp-httplib), [WebSocket client/threading/timeout documentation](https://github.com/yhirose/cpp-httplib/blob/master/README-websocket.md).
- **[U3] fmt:** [latest release 12.2.0](https://github.com/fmtlib/fmt/releases/latest), [9.1 format/runtime API](https://fmt.dev/9.1.0/api.html).
- **[U4] RapidJSON:** [latest tagged release v1.1.0](https://github.com/Tencent/rapidjson/releases/latest), [official repository](https://github.com/Tencent/rapidjson). Local post-tag source pin is recorded in §5.
- **[U5] uWS:** [hoytech fork and visible activity](https://github.com/hoytech/uWebSockets), [current mainline architecture](https://github.com/uNetworking/uWebSockets).
- **[U5s] uWS inspected source:** [v0.14.8 Hub.h](https://raw.githubusercontent.com/hoytech/uWebSockets/v0.14.8/src/Hub.h), [Hub.cpp](https://raw.githubusercontent.com/hoytech/uWebSockets/v0.14.8/src/Hub.cpp), [Group.h](https://raw.githubusercontent.com/hoytech/uWebSockets/v0.14.8/src/Group.h), [Group.cpp](https://raw.githubusercontent.com/hoytech/uWebSockets/v0.14.8/src/Group.cpp), [Node.cpp](https://raw.githubusercontent.com/hoytech/uWebSockets/v0.14.8/src/Node.cpp), [Node.h](https://raw.githubusercontent.com/hoytech/uWebSockets/v0.14.8/src/Node.h), [master Node.cpp](https://raw.githubusercontent.com/hoytech/uWebSockets/master/src/Node.cpp), [master Node.h](https://raw.githubusercontent.com/hoytech/uWebSockets/master/src/Node.h). Installed uWS is external and was unavailable; these define the documented implementations, not an observed local binary.
- **[U6] OpenSSL:** [current downloads/branches](https://openssl-library.org/source/), [release strategy/support dates](https://openssl-library.org/policies/releasestrat/). 4.0.3 stable, 3.5.9 LTS observed; 4.1 beta deliberately excluded from recommendation.
- **[U7] zlib:** [official releases/changelog summary](https://zlib.net/), 1.3.2 observed.
- **[U8] libuv:** [stable release v1.53.0](https://github.com/libuv/libuv/releases/latest).
- **[U9] rapidcsv:** [release v9.07](https://github.com/d99kris/rapidcsv/releases/latest).
- **[U10] PicoSHA2:** [official API/project](https://github.com/okdshin/PicoSHA2); no unverified latest-version claim.
- **[U11] uri-parser:** [official repository/requirements/activity](https://github.com/bhumitattarde/uri-parser).
- **[U12] GoogleTest:** [release v1.18.0 and compiler policy](https://github.com/google/googletest/releases/latest).
- **[U13] fixtures:** [official Zerodha mocks](https://github.com/zerodha/kiteconnect-mocks).
- **[U14] docs:** [Doxygen download 1.18.0](https://www.doxygen.nl/download.html), [theme release v2.5.0](https://github.com/jothepro/doxygen-awesome-css/releases/latest).
- **[U15] CI:** [actions/checkout v7.0.1](https://github.com/actions/checkout/releases/latest).
- **[U16] API contracts:** [Kite WebSocket protocol and limits](https://kite.trade/docs/connect/v3/websocket/), [session checksum and Authorization](https://kite.trade/docs/connect/v3/user/), [mutual-fund API](https://kite.trade/docs/connect/v3/mutual-funds/).
- **[U17] Candidate async transport:** [Boost release 1.92.0](https://www.boost.org/releases/latest/), [Beast WebSocket construction/TLS/thread-safety](https://www.boost.org/doc/libs/latest/libs/beast/doc/html/beast/using_websocket.html).
- **[U18] Candidate JSON:** [nlohmann/json release 3.12.0](https://github.com/nlohmann/json/releases/latest).

## 26. Migration Execution Update — Phase 1

**Status:** PARTIALLY COMPLETED. This update preserves the original audit findings and records implementation evidence from the first migration phase.

### Implemented

- Added the REST-only public header `include/kitepp/rest.hpp`; the supported umbrella now composes REST and ticker headers explicitly.
- Removed the `KITE_UNIT_TEST` conditional class layout. `kite::sendReq` is now a stable virtual protected seam in every translation unit, with a virtual destructor for safe polymorphic test ownership.
- Made test fixture paths use the CMake-defined `KITE_TEST_DATA_DIR`, allowing tests to run from an external build directory or arbitrary process working directory.
- Separated REST test compilation from ticker/uWS headers. Ticker and umbrella consumer checks receive uWS requirements explicitly.
- Added C++17 consumer checks for the umbrella, REST-only, ticker-only, standalone headers, and multiple translation units.
- Added phase-one characterization/protection tests covering malformed JSON syntax, numeric boundaries, DOM array consumption, subscription serialization, HTTP-200 error envelopes, bracket-order endpoint formatting, exception reporting, credentials, and the SIP fixture contract.

### Finding status changes

- **F06 — PARTIALLY FIXED:** the test-only ODR/class-layout hazard is removed and REST-only inclusion is available. Public RapidJSON/httplib coupling remains open.
- **F28 — PARTIALLY FIXED:** fixture location is independent of the working directory, the REST/ticker test boundary is improved, and consumer compile checks exist. Production transport, TLS, final-wire, and lifecycle coverage remain incomplete.
- **F03 — PARTIALLY FIXED:** the REST test no longer requires the ticker include path; external uWS version/backend/ABI discovery remains unresolved.
- **F22 — OPEN and sanitizer-confirmed:** ASan/UBSan execution reports the uWS `Group` allocation created by `ticker` and not released during the existing binary parsing test.
- **F16, F17, F18, F19, F20, F25, F26, F28 — OPEN:** phase-one characterization tests or baseline evidence now make selected behaviors reproducible, but corrective implementation is reserved for later planned phases.

### Verification boundary

The normal C++17 build and CTest suite pass using GCC 13.3, OpenSSL 3.0.13, zlib 1.3, a temporary uWS v0.14.8 build, and a temporary GoogleTest/GoogleMock 1.10.0 build. The sanitizer build compiles and the REST test passes, but the ticker test fails LeakSanitizer on the known uWS Group ownership defect. No C++20, dependency upgrade, package target, or ticker replacement was performed.

## 27. Migration Execution Update — Phase 2 and Phase 1 Remediation

**Status:** COMPLETED for the available Linux/AArch64 C++17 checkpoint. This
appendix supersedes Phase 1's open leak/lifecycle gate without deleting its
historical evidence. Full migration/release acceptance remains open.

### Implemented and verified

- **F08, F11, F12, F13, F14, F17, F20, F22, F24, F26:** corrected credential
  coherence, literal command serialization, explicit replay, cancellable retry,
  terminal shutdown, diagnostic lifetime, SIP ID, session ownership, exception
  `what()`, and peer/hostname TLS validation in the exercised paths.
- **F07/F16/F18/F19/F21/F23/F25/F28/F31 — PARTIALLY FIXED:** initialized and
  validated required fields; checked JSON/CSV/candle/frame boundaries; defined
  numeric/missing-field policy; non-consuming DTO arrays; unsigned component
  encoding; documented owner/callback contract and corrected envelope handling.
  Public parser coupling/defaults, legacy internal transfer helper, general REST
  URL/date semantics and wider transport/network policy are still open.
- Ticker uses inline Beast/Asio with Boost 1.83; the approved transport-forward
  adjustment is in `plan.md`. No vendored gitlink was changed. uWS/libuv are no
  longer runtime requirements, although old build/CI scaffolding awaits Phase 3.

### Verification boundary

GCC 13.3 / Linux AArch64 / OpenSSL 3.0.13 / Boost 1.83 / GTest 1.10:
external and repository-child CTest 3/3; independent consumers 5/5; all four
examples built; arbitrary-cwd REST/binary tests and five consumer executables
passed; ASan/UBSan CTest 3/3 with leak and stack-use-after-return checks passed.
Local TLS/WSS tests reject wrong-host, expired and untrusted certificates, validate
replay/callback copies, and cover cancellation/backoff/close/queue limits.
Beast's lingering internal upgrade timer was detected and explicitly disarmed.

NOT RUN — TSan: even an empty executable aborts with `unexpected memory mapping`.
NOT RUN — Clang/macOS/Windows: not available in this checkpoint.
NOT RUN — live Kite service: tests use synthetic local credentials/certificates.
F01/F02/F03 packaging and F05 C++20 fmt work remain later gates. Runtime policy
and limitations are recorded in `docs/runtime_contract.md` and the journal.

## 28. Migration Execution Update — Phase 3

**Status:** COMPLETED for local Linux/AArch64 packaging qualification.

- **F01/F02/F03 — FIXED in the exercised package/build paths:** target-local
  requirements, default header compilation, component INTERFACE targets,
  install/export/config/version metadata and real transitive external targets.
  Minimum CMake 3.18.4 and current 4.4.4 each pass CTest 9/9; 3.28.3 also passes.
- **F04 — PARTIALLY FIXED:** recorded SDK headers/gitlink identities are checked;
  bootstrap no longer clones moving uWS refs or downloads into source. CI action,
  CMake tools and Docker base use immutable identities. System-package versions
  still follow provider security updates and are recorded at qualification time.
- **F06/F28 — PARTIALLY FIXED:** uniform TLS/header-only fmt configuration,
  independent/per-header and REST/ticker mixed-order multi-TU checks added. Models
  remain HTTP/CSV/fmt/RapidJSON-coupled and are labeled accordingly.
- **F00 — PARTIALLY FIXED:** current build/runtime guides supersede historical
  uWS README/mainpage setup; full narrative/example consolidation remains Phase 7.

Manual, subdirectory and relocated independent consumers pass 6/6 each; REST-only
subdirectory and installed consumers pass 3/3 without Boost. Unknown/unavailable
required components and late incompatible third-party configuration are rejected;
an unavailable optional ticker does not break core discovery. ASan/UBSan passes
9/9 after configuration changes. Installs contain no SDK binary or SDK `.cpp`.
Docker build and offline smoke pass on AArch64. Hosted CI/macOS/Clang are NOT RUN
at this checkpoint; a workflow file is not evidence that those lanes passed.

## 29. Migration Execution Update — Phase 4

**Status:** COMPLETED for the local C++17 dependency qualification checkpoint.

cpp-httplib 0.59, fmt 12.2, rapidcsv 9.07 and external GTest 1.18 were qualified
independently; exact commits/hashes and isolated tests are in the journal. OpenSSL
>=3.0 uses supported provider/backport policy; current Ubuntu package is patched
3.0.13-0ubuntu3.16. Boost 1.83 remains the tested floor. RapidJSON/PicoSHA2 unchanged.

**F05 — implementation fixed, C++20 matrix pending:** runtime login formatting
explicitly uses fmt::runtime; no runtime ticker URL format remains.
**F04/F28/F31 — PARTIALLY FIXED:** immutable source locks, expanded actual wire,
read-timeout and provider compatibility tests. New provider container/query
behavior is adapted without weakening original goldens. Raw plus correctly stays
literal via `%2B`; spaces use `%20`. General identifier/datetime validation,
high-level configurable REST roots and all-platform coverage remain open.

Combined minimum-CMake C++17 headers/examples and CTest PASS 9/9; updated-provider
ASan/UBSan PASS 9/9; independent installed consumers PASS 6/6. The optional
per-header generator was repaired for CMake 3.18 after an actual failure. SDK
targets remain INTERFACE and no SDK binary is installed. C++20/platform/performance
release gates are not yet claimed complete.

## 30. Migration Execution Update — Phase 5

**Status:** PARTIALLY COMPLETED — local Linux/AArch64 C++20 checkpoint passes;
complete compiler/OS acceptance remains pending.

**F05 — locally verified:** explicit runtime fmt boundaries compile in C++20
under GCC 13.3, Clang 18.1.3/libstdc++ 13 and Clang/libc++ 18. SDK targets export
C++20 by default and retain an explicit C++17 rollback feature/macro configuration.
**F29 — private copies reduced, production effect unmeasured:** checked span
frame/packet views and endian/stack field decoding remove temporary payload/field
allocations. Synthetic full-packet allocations are 62→12, 940→170, 3728→654 for
1/16/64 packets. Returned models/callback copying still own their data.

The matrix exposed libc++ 18's missing floating from_chars overload; detection and
classic-locale direct-target parsing preserve tested decimal/finite/range policy.
New regressions cover global locale, subnormals, malformed suffixes and precision
without intermediate double rounding. Sanitizer shutdown timing included 1000
command constructions; the test now measures stop-to-return with a stalled peer,
without relaxing the shutdown assertion or changing SDK shutdown code.

Normal/rollback and GCC/Clang sanitizer suites pass 9/9; libc++ headers/examples
pass. Independent manual/subdirectory/relocated consumers pass 6/6, REST-only
installed consumers 3/3, rollback installed consumers 6/6. CMake 3.18 propagates
C++20 through relocated exports even for a consumer requesting C++11. All SDK
targets remain INTERFACE, without compiled SDK source/binary. CI adds explicit
rollback and matching-libc++ provider lanes; YAML/shell validation passes.

NOT RUN — hosted macOS/Windows/newer compiler/other architecture lanes: providers
or runners unavailable. TSan host runtime failure remains. The journal retains
exact commands, initial failures/timeouts and measurement limitations; no complete
migration/release or production-performance acceptance is claimed.

## 31. Migration Execution Update — Additional Phase 5 Qualification

**Status:** PARTIALLY COMPLETED; additional available Linux/AArch64 gates pass.

GCC 14.2 and Clang/libc++ 20.1.2 full C++20 headers/examples/offline groups pass
9/9 with distro GTest/GMock 1.14. Clang/libc++ 20 ASan/UBSan/leak groups pass 9/9;
GCC 14 and Clang 20 relocated consumers pass 6/6 each. SDK locks are unchanged.

The intended libc++ 18 sanitizer lane failed 8/9 with an allocation mismatch in
callback exception destruction. A program using only stdexcept reproduces it;
the same unsuppressed probe passes with libc++/libc++abi 20. CI selects that
qualified sanitizer provider, preserves 18 normal checks and does not suppress
allocator errors or change SDK exception containment to hide the failure.

Package qualification found an actual optional-component gap: missing Boost
rejected REQUIRED rest + OPTIONAL ticker in a full install. The config now
distinguishes required/optional streaming discovery. CMake 3.18/4.4 optional
missing/present-provider cases pass; required ticker/umbrella/unknown cases still
fail with expected diagnostics. Rollback installed consumers remain PASS 6/6.
CI retains this component regression and adds newer provider lanes (eight total).
No SDK implementation binary/source or new public DTO/result API was introduced.

## 32. Migration Execution Update — Phase 7 Local Release Qualification

**Status:** COMPLETED for the available Linux/AArch64 release checkpoint;
cross-platform release acceptance remains pending.

Replaced the stale README and Doxygen mainpage narrative with current C++20,
Boost.Asio/Beast, CMake component/package, TLS, owner-thread, borrowed-payload,
shutdown, dependency and compatibility guidance. Added installed `release-notes.md`
with downstream rebuild/security checklist. Removed obsolete uWS setup commands
from user-facing guides. README/mainpage no longer claim C++17 as the default.

Examples now validate required environment variables and never print access tokens.
`example1` still demonstrates interactive request-token exchange but only stores the
resulting token in memory; examples 3/4 reject missing key/token before ticker setup.
Added `kitepp-documented-examples`, which compiles the README REST/ticker snippets
without contacting Kite. CMake builds four live examples plus this check under
C++20 and the explicit C++17 rollback; missing-credential smoke runs return 2 for
examples 1/3/4, while the commented documentation example exits successfully.

Doxygen configuration now includes build/dependency/runtime/release pages, uses an
absolute mainpage path, enables Graphviz class/dependency graphs and suppresses
only undocumented-member noise. Doxygen 1.9.8 + Graphviz successfully generates
HTML with `index.html` and release-notes page output. Legacy API comments retain
non-fatal duplicate `ex1` paragraph/stale parameter warnings; this is recorded
technical debt rather than hidden as a source/API change. The CI documentation job
installs Doxygen/Graphviz and builds `docs` with ticker disabled.

Current C++20 phase7 build has BUILD_TESTS/BUILD_EXAMPLES/KITEPP_CHECK_HEADERS/
KITEPP_BUILD_BENCHMARKS ON: CTest PASS 9/9; examples, documented snippets,
headers and benchmark compile. C++17 phase7 examples/documentation/header checks
also compile. C++20 decoder benchmark checksums pass with allocations 12/170/654;
C++17 comparison checksums pass with allocations 62/940/3728. Timing remains
shared-host diagnostic data, not production evidence. Current installed C++20
package includes release notes; independent relocated consumer CTest PASS 6/6.
Manual header language probes pass with C++20 and explicit C++17 compatibility.

The generated docs build was run in the existing pinned Ubuntu container with
Doxygen 1.9.8, Graphviz 2.42.2 and GCC 13.3; no generated HTML was committed.
No live API calls, release upload, commit or push were performed. macOS/Windows,
hosted CI, live service, TSan and production performance remain outside this local
checkpoint; the migration is not called a complete released artifact.

Hosted macOS/other-platform evidence is still unavailable; gh authentication is
absent and no cross-architecture interpreter is configured. These facts do not
alter the plan's remaining platform acceptance gate. See the append-only journal
for exact commands, image/provider versions, the standalone probe and failures.
