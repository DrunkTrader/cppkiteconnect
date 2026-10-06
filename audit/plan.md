# cppkiteconnect — C++20 Header-Only Migration Plan

**Baseline:** `d437b35300bfd13f9de10632aeb19410720c8ad7`  
**Branch:** `migration-branch`  
**Plan status:** Phases 1–4 completed on the available Linux/AArch64 toolchain. Phase 5 remains partially completed pending the declared cross-platform matrix. Phase 7 local documentation, example, benchmark and package work is completed; release acceptance remains gated by that matrix.

## 1. Objective

Modernize cppkiteconnect into a maintainable, reproducible, C++20 header-only SDK while preserving the existing high-level REST and ticker APIs wherever practical.

The migration must improve correctness, parser safety, TLS trust, ticker shutdown, build consumption, and dependency reproducibility without combining unrelated API, transport, JSON, and language changes into one unreviewable rewrite.

Header-only SDK ownership is a hard constraint. The project must not introduce project-owned `.cpp` implementation files, a compiled SDK library, a compiled pImpl, or an SDK-specific runtime library.

## 2. Decisions

### REST transport

Retain synchronous cpp-httplib for REST initially. The current public API is synchronous, and an asynchronous HTTP rewrite is not justified by the audit. Isolate request construction and transport behind a testable internal seam, then upgrade cpp-httplib independently after the C++17 baseline is protected.

### Ticker transport

Replace the legacy uWS lifecycle and transport behind an internal adapter. Boost.Beast/Asio is the primary candidate because it supports an explicit owner loop, cancellable timers and socket operations, and peer-authenticated TLS. A newer cpp-httplib WebSocket client remains a qualification alternative for a deliberately blocking implementation, but it must first prove that pending connect/read operations can be interrupted safely.

The selected backend must pass the same lifecycle, TLS, replay, cancellation, callback, malformed-input, and leak tests. The backend is not selected merely because it is modern or available.

### Public ticker behavior

Preserve the existing `connect()` → `run()` usage model, `stop()`, the nine callback fields, callback payload ownership rules, and owner-thread callback delivery initially.

The redesigned ticker uses a single owner thread: ticker state, subscriptions, callback mutation, transport operations, and callback delivery occur on the `run()` thread. Cross-thread stop requests and status snapshots may be supported safely. Arbitrary cross-thread calls to `subscribe`, `unsubscribe`, `setMode`, credential setters, or callback fields are not part of the required contract.

Copy and move semantics should be explicitly disabled where transport/session state requires stable ownership. Destruction requires the event loop to be stopped and quiescent; destruction during a callback or while another thread runs the loop is unsupported.

### Platform baseline

The modernized C++20 dependency set targets 64-bit systems. Initial validation lanes are Linux GCC/Clang and macOS AppleClang/libc++. Windows remains a separate qualification target rather than an implied support guarantee. This permits evaluation of current cpp-httplib releases, whose upstream support policy does not include 32-bit platforms.

### JSON and models

Retain RapidJSON initially. First harden and isolate the codec layer, validate all input boundaries, and preserve public RapidJSON constructors/parsers through compatibility adapters. Do not replace JSON as part of the C++20 language migration.

Longer-term JSON replacement is a separate decision requiring schema, numeric, ownership, exception, allocation, and source-compatibility evidence.

### Error model

Preserve the current exception-facing API and named exception hierarchy at the first compatibility boundary. Internally classify transport, HTTP, envelope, schema, and callback failures. Add structured or result-returning APIs only as additive interfaces; do not perform a blanket `std::expected` rewrite. `std::expected` is C++23 and is not required for this C++20 migration.

### Build and packaging

Introduce namespaced CMake `INTERFACE` targets, initially along these lines:

- `kitepp::models`
- `kitepp::rest`
- `kitepp::ticker`
- `kitepp::kitepp` umbrella target

Targets must express the actual dependency closure. Models must not be advertised as transport-independent until the include graph actually makes them so. Installation and package discovery must not download dependencies or mutate the source tree.

Use a provisional CMake minimum of 3.18, followed by an oldest-version configure/build validation. CMake 3.24 is not required solely for the planned interface targets and ordinary install/export package behavior.

### Dependency policy

Keep pinned vendored headers initially and preserve recorded submodule revisions. Support external package/config targets where practical, but do not make consumers depend on undocumented build-machine paths. Normalize include paths and macros before claiming broad external-provider support.

No mandatory FetchContent, recursive source downloads, or dual Conan/vcpkg requirement should be introduced. Any optional dependency acquisition mechanism must be explicit, immutable, and isolated from normal package consumption.

## 3. Finding qualifications

Most audit findings are source-confirmed. The following qualifications must remain attached to the migration record:

- The C++20 fmt failure was historically reproduced with GCC 13.3 and fmt 9.1, but is compiler/configuration dependent rather than universal. Runtime format strings must still be handled explicitly before enabling C++20.
- `%2B` is correct encoding for a literal plus. The defect is the unclear date/time input contract and lack of final-wire tests, not percent encoding by itself.
- Parser boundary and lifetime findings are confirmed. RapidJSON assertions, binary undefined behavior, and ordinary parse errors must be tested and classified separately.
- The SDK's unnecessary tick-parser copies are confirmed by source inspection, but production performance impact is unmeasured and must not be presented as a benchmark result.
- Subscription replay currently omits explicit subscribe actions. Whether mode-only behavior happens to work against a live server is unverified.
- uWS group ownership, timer shutdown, and WebSocket TLS consequences are confirmed omissions in the SDK wrapper; leak/hang/trust behavior depends on the actual deployed uWS revision and build.
- The audit diagnostics recorded in `audit_report.md` were historical checks. They are evidence for planning, not a claim that the checks were rerun during this plan.

## 4. Migration order

### Phase 0 — Audit and decision record — complete

Preserve the inspected source and dependency pins. Record the current architecture, findings, historical diagnostics, external dependency assumptions, and planning decisions. Do not modify production source during this phase.

### Phase 1 — Reproducible C++17 baseline and protection tests

**Objective:** establish a trustworthy baseline before changing the standard or transport.

**Work:**

- Reproduce the existing C++17 build and test suite with initialized pinned submodules and documented external dependencies.
- Make fixture resolution independent of the process working directory.
- Separate REST-only compilation from ticker/uWS compilation.
- Add supported-umbrella, REST-only, ticker-only, standalone-header, two-translation-unit, and external-consumer compile checks.
- Replace the `KITE_UNIT_TEST` class-layout alteration with a stable test seam.
- Add regression tests for dangling diagnostics, malformed JSON/CSV/binary input, numeric boundaries, DOM immutability, subscription serialization, authentication changes, SIP response parsing, exception messages, response envelopes, URL/final-target construction, and callback payload lifetimes.
- Add ASan and UBSan jobs where the toolchain supports them.

**Exit criteria:** offline tests run from both a repository-child and external build tree; consumer smoke builds validate the actual headers; known defects have stable regression tests; missing external prerequisites are distinguished from code failures.

**Rollback:** retain the original C++17 build path and revert only test/build-infrastructure changes that prevent baseline verification.

### Phase 2 — Safety, security, and ticker lifecycle stabilization

**Objective:** correct confirmed defects and define an explicit runtime contract while preserving the public facade.

**Work:**

- Fix static diagnostic lambdas that capture stack parameters.
- Validate JSON root/envelope/member types, CSV row widths, candle widths, binary packet counts/lengths, arithmetic, and recognized packet sizes.
- Define numeric and missing-field semantics without silently accepting malformed data.
- Initialize required request scalars and validate incomplete requests deterministically.
- Derive REST authorization from coherent current credentials; test token/key replacement.
- Correct SIP modification response parsing, `libException::what()`, and inconsistent response classification while documenting compatibility behavior.
- Correct subscribe/unsubscribe serialization and validate mode/token/count limits.
- Implement explicit subscribe-before-mode replay from desired state.
- Define ticker states: `Ready → Connecting → Open → Backoff → Stopping → Stopped`.
- Replace callback-thread sleeping with cancellable retry deadlines, capped delay arithmetic, and explicit permanent-failure handling.
- Make `stop()` idempotent and terminal: cancel retry/connect/read/write work, stop maintenance timers, prevent future reconnects, and complete safe close/drain ordering.
- Contain user callback exceptions according to a documented policy and prevent recursive error reporting.
- Require WSS peer certificate and hostname verification with configurable trust roots.

**Exit criteria:** malformed input cannot cause unchecked reads or parser assertions in tested paths; valid subscription commands and replay traces are verified; stop works during every lifecycle state; callbacks obey documented thread/lifetime rules; TLS trust tests reject wrong-host, untrusted, and expired certificates.

**Rollback:** retain the old public facade behind the existing transport only if it passes the new security/lifecycle acceptance tests. Do not release the legacy WSS path as secure merely because it remains source-compatible.

### Phase 3 — Build-system and dependency boundaries

**Objective:** make the header-only SDK consumable as a real CMake package.

**Work:**

- Replace global include paths and global standard settings with target-local usage requirements.
- Add namespaced `INTERFACE` targets and an umbrella target.
- Make REST, ticker, and model headers match their actual dependency closures.
- Normalize vendored and installed third-party include layouts through adapters where needed.
- Scope OpenSSL/fmt/httplib configuration macros consistently before headers are included.
- Add install/export/package-config files with relocatable paths.
- Add `add_subdirectory` and installed-package consumer smoke projects.
- Add immutable dependency identity checks and update CI/container bootstrap without source-tree downloads.
- Correct CI triggers so migration branches and pull requests receive validation.

**Exit criteria:** default consumer targets compile SDK headers; exported transitive requirements work from a relocated install; component targets do not accidentally pull in ticker/uWS; CMake 3.18 and current CMake lanes agree.

**Rollback:** preserve manual include usage and the existing repository build while package targets are validated; do not remove old entry points until consumer tests pass.

### Phase 4 — Controlled dependency and transport changes

**Objective:** update dependencies and replace the streaming backend without conflating changes.

**Work:**

- Refresh OpenSSL support policy and verify package-provided trust stores.
- Upgrade cpp-httplib, fmt, rapidcsv, and GTest independently against golden tests.
- Correct all fmt runtime-format calls before any C++20 target is enabled.
- Prototype Beast/Asio ticker transport against the lifecycle adapter contract.
- Qualify the alternative blocking WebSocket client only if it can safely interrupt connect/read/close operations and satisfy the owner-loop contract.
- Remove uWS/libuv only after the selected replacement passes TLS, cancellation, replay, callback, malformed-input, and destruction tests.
- Keep RapidJSON and PicoSHA2 unless separate evidence justifies replacing them.

**Exit criteria:** every dependency change has isolated passing baseline, wire, TLS, and lifecycle evidence; external binaries are compatible with the declared 64-bit/toolchain policy; removed dependencies no longer link accidentally.

**Rollback:** dependency changes are individually revertible. Retain the previous adapter behind a build option only during qualification, never as an unverified production fallback.

### Phase 5 — C++20 language migration

**Objective:** make C++20 an explicit, tested target contract after behavior is stabilized.

**Work:**

- Apply target-level `cxx_std_20`, required standard, and disabled-extension settings.
- Fix fmt compile-time/runtime format-string boundaries.
- Use `std::span` for validated binary views, `std::endian` for byte-order decisions, concepts where they improve diagnostics, and `steady_clock` for retry/liveness timing.
- Avoid modules, coroutines, `std::format`, `std::expected`, and broad ranges rewrites unless separately justified.
- Test oldest supported compiler/standard-library combinations and current compilers on declared 64-bit platforms.

**Exit criteria:** public headers, installed consumers, examples, tests, sanitizers, and selected ticker integration tests pass under C++20; wire outputs, models, callback ordering, and error compatibility match the stabilized baseline.

**Rollback:** keep the C++17 branch/target available until the complete C++20 matrix and downstream rebuild guidance are accepted.

### Phase 6 — Versioned API and model modernization

Defer strong IDs, validated request factories, explicit optional data states, consistent timestamps/quantities, result-returning overloads, and removal of public RapidJSON coupling until the C++20 baseline is released and compatibility impact is reviewed.

Breaking changes must be grouped in a major version with migration examples and an API/source diff. Existing mutable DTOs, fluent setters, exception types, callback fields, and owning return values remain compatibility surfaces until then.

### Phase 7 — Measured performance, documentation, and release

After safety gates, benchmark binary decoding, allocation count, latency, retry behavior, and optional HTTP keep-alive. Optimize with bounded byte views, reservations, and moves only where measurements justify it.

Compile all documented examples, remove token printing, document the 64-bit/toolchain/platform matrix, TLS trust policy, callback affinity, borrowed payload lifetimes, shutdown, errors, and dependency setup. Finish install/package acceptance and release notes.

## 5. Acceptance gates

The migration is complete only when all of the following are true:

1. The C++17 baseline and C++20 target are reproducible from clean external build trees.
2. Header-only consumers compile in single- and multi-translation-unit programs without test-only class definitions or ODR-sensitive macro mismatches.
3. REST final URLs, headers, form bodies, JSON bodies, authentication changes, error envelopes, TLS failures, and timeout behavior have wire-level tests.
4. Binary, JSON, CSV, and text parsers reject malformed/truncated inputs deterministically and pass sanitizer coverage.
5. Ticker replay sends valid subscribe commands before mode commands and preserves desired state across reconnects.
6. Ticker stop is idempotent, cancellable, terminal, and safe during connecting, backoff, open, callback, and disconnect states.
7. WSS certificate-chain and hostname verification are demonstrated with trusted and intentionally invalid local certificates.
8. Callback thread affinity, reentrancy, borrowed argument lifetime, callback exceptions, and destruction rules are documented and tested.
9. Installed/exported CMake targets carry all required transitive dependencies without hard-coded build paths or implicit downloads.
10. The declared 64-bit compiler/OS matrix passes with supported dependency/provider combinations.
11. Wire outputs, public models, callback ordering, and compatibility exceptions match approved golden behavior except for explicitly documented corrections.
12. Performance changes are supported by measurements and do not weaken validation, ownership, cancellation, or compatibility gates.

## 6. Immediate next implementation step

Complete the remaining **Phase 5** compiler/OS qualification using the configured
CI lanes, especially macOS AppleClang/libc++. Local C++20 and C++17 rollback
checkpoints pass. Retain the explicit C++17 compatibility configuration until the
complete matrix and downstream rebuild guidance are accepted. The original
Phase 1 starting instruction and remediation history are preserved in the journal.

`audit_report.md` remains the historical audit record. `architecture.md` remains the source-derived current architecture with future proposals labeled accordingly. This document records the selected strategy and implementation order.

## 7. Execution status

Phases 1–4 are **COMPLETED** for the available Linux/AArch64 C++17 checkpoint.
Repository-child and external offline suites, independent header consumers,
arbitrary-cwd tests, and ASan/UBSan with leak checking pass. The approved Beast
replacement removes the legacy Group leak. Local TLS/WSS tests cover trust,
hostname/expiry rejection, replay, callback ownership/exceptions and terminal
cancellation. See the append-only journal for commands, failures and limits.
Phase 3 adds real component targets, consistent configuration and relocatable
exports. CMake 3.18.4, 3.28.3 and 4.4.4 offline suites pass; manual/subdirectory/
relocated consumers and REST-only discovery pass, with no SDK binary installed.
Phase 4 independently qualifies cpp-httplib 0.59.0, fmt 12.2.0, rapidcsv 9.07
and external GTest 1.18, with supported distro OpenSSL >=3.0. Original parameter
and URI encoding behavior is preserved at the REST adapter boundary. Combined
C++17 headers/examples/wire/TLS/lifecycle/sanitizer/package checkpoints pass.
This checkpoint is not a declaration that the complete C++20 migration or
cross-platform release gates have passed.

Phase 5 is **PARTIALLY COMPLETED**. The default contract is C++20; an explicit
`KITEPP_CXX_STANDARD=17` rollback exports the C++17 feature and compatibility macro.
Linux/AArch64 GCC 13.3/14.2 and Clang 18.1.3/20.1.2 checkpoints pass;
GCC/Clang libstdc++ and Clang/libc++ 20 ASan/UBSan/leak suites pass. libc++ 18
normal checks pass, but its Ubuntu exception allocator fails an SDK-independent
ASan probe, so the qualified libc++ sanitizer lane uses LLVM 20 without suppression.
Manual, subdirectory, relocated and REST-only consumers pass; minimum CMake
propagates C++20 even when a consumer asks for C++11. Checked span/endian decoding
reduces measured synthetic allocations without changing owning callback data.
The libc++ floating-from_chars gap is handled with a tested classic-locale
fallback. Full platform/compiler acceptance, hosted CI and Phase 7 release work
are still open; an available Linux lane is not proof of macOS/Windows support.
Optional ticker discovery now tolerates absent Boost while retaining required
REST/model targets; minimum/current-CMake and C++17/C++20 package checks pass.

Phase 7 local release/documentation work is **COMPLETED FOR THE AVAILABLE
LINUX/AARCH64 CHECKPOINT**. README and Doxygen mainpage now describe C++20,
Beast/Asio, the package targets, TLS/runtime ownership contract and current
limitations. Historical uWS build instructions were removed from user-facing
guides. Examples validate credentials without printing access tokens; all four
live examples and dedicated README/Doxygen snippets compile in C++20 and the
protected C++17 configuration. Doxygen 1.9.8 with Graphviz generates HTML output.
The release notes, downstream checklist and package-installed documentation are
included. Decoder allocation probes run in both standards, and the final C++20
package consumer passes 6/6. The SDK is not declared released: hosted macOS,
Windows and other platform matrix evidence remains pending, and live-service/
production-performance qualification is outside the offline release checkpoint.

### Approved remediation and sequencing correction

The user authorized fixing the gate and continuing. Phase 1 is a characterization
baseline: expected pre-existing safety findings are to be reproduced and tracked,
not silently suppressed or required to be fixed before the phase that fixes them.
Its outstanding protection tests will be completed alongside the corresponding
Phase 2 regressions, with both checkpoints reverified before Phase 3.

The legacy uWS public API cannot cancel a pending `Hub::connect()` or configure
peer/hostname verification. Consequently Phase 2's terminal shutdown and TLS
criteria cannot be met by wrapper-only changes. Bring forward the Beast/Asio
ticker replacement from Phase 4 into Phase 2, as permitted by the audit's
Phase 2 transport exception. Phase 4 still handles independent REST/format/CSV/test
dependency refreshes; it will qualify the already-implemented ticker adapter.
The SDK remains header-only and C++17 during this work. Boost 1.83 is the initial
locally available qualification baseline; no claim of a tested Boost 1.92 floor
is made. Record exact external provider versions in the journal.
