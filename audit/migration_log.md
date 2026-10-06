# cppkiteconnect Migration Log

**Baseline:** `d437b35300bfd13f9de10632aeb19410720c8ad7`  
**Branch:** `migration-branch`

# Phase 0 — Audit and decision record

## Status
COMPLETED

## Objective
Preserve the baseline and document the current architecture, findings, constraints, dependency assumptions, and selected migration strategy.

## Plan Reference
`audit/plan.md`, §4, Phase 0 — Audit and decision record.

## Changes Made

- Existing source and dependency pins were inspected without changing the baseline.
- `audit/audit_report.md` records source findings and historical diagnostics.
- `audit/architecture.md` records current and future architecture separately.
- `audit/plan.md` records the selected header-only C++20 migration strategy.

## Files Changed

- `audit/audit_report.md`
- `audit/architecture.md`
- `audit/plan.md`

## Dependencies Changed

- None

## API Changes

- None

## Tests / Verification

```text
Historical audit diagnostics are recorded in audit/audit_report.md.
```

### Results

```text
Planning completed. The repository was not modified during the audit/planning work.
```

## Header-Only Verification

The audit confirmed that first-party SDK implementation exists in headers and no project-owned SDK library or source implementation target exists.

## Problems Discovered

- The complete list is recorded in `audit/audit_report.md`.

## Decisions Made

- Stabilization-first migration.
- Synchronous cpp-httplib REST initially.
- Beast/Asio as primary ticker replacement candidate.
- Owner-thread ticker contract.
- 64-bit modernized dependency baseline.

## Remaining Work

- All implementation phases.

## Acceptance Criteria

- [x] Current architecture documented.
- [x] Migration sequence documented.
- [x] Header-only constraint retained.

## Git Commit

None.

## Notes

External dependency observations were distinguished from locally executed tests.

# Phase 1 — Reproducible C++17 baseline and protection tests

## Status
PARTIALLY COMPLETED

## Objective
Establish a reproducible C++17 baseline, make fixture resolution independent of the working directory, separate REST-only and ticker/uWS compilation, remove the test-only class-layout switch, and add protection/consumer checks.

## Plan Reference
`audit/plan.md`, §4, Phase 1 — Reproducible C++17 baseline and protection tests.

## Changes Made

- Added `include/kitepp/rest.hpp` as a REST-only public entry point.
- Changed `include/kitepp.hpp` to compose the REST-only header and ticker header.
- Made `kite::sendReq` virtual in the ordinary production class definition and added a virtual destructor; removed the `KITE_UNIT_TEST` conditional layout.
- Added CMake-provided fixture path resolution through `KITE_TEST_DATA_DIR`.
- Updated REST and ticker tests to use source-root fixture paths rather than process-cwd-relative paths.
- Added REST-only, ticker-only, umbrella, standalone-header, and multi-translation-unit consumer executables.
- Added phase-one characterization/protection tests for malformed JSON syntax, numeric limits, DOM mutation, subscription serialization, HTTP-200 error envelopes, endpoint formatting, exception reporting, credential accessors, and the SIP fixture contract.
- Corrected external uWS link ordering and linked zlib for consumer targets.

## Files Changed

- `CMakeLists.txt`
- `AGENTS.md`
- `include/kitepp.hpp`
- `include/kitepp/rest.hpp`
- `include/kitepp/kite.hpp`
- `tests/unit/kitepp.hpp`
- `tests/unit/utils.hpp`
- `tests/unit/test_paths.hpp`
- `tests/unit/tickertest.cpp`
- `tests/unit/kite/phase1.cpp`
- `tests/compile/umbrella.cpp`
- `tests/compile/rest.cpp`
- `tests/compile/ticker.cpp`
- `tests/compile/standalone.cpp`
- `tests/compile/two_tu_a.cpp`
- `tests/compile/two_tu_b.cpp`
- `tests/compile/two_tu_main.cpp`

## Dependencies Changed

- None in the repository.
- Temporary validation dependencies were built outside the repository: uWS v0.14.8 and GoogleTest/GoogleMock 1.10.0.

## API Changes

- Added the public REST-only header `kitepp/rest.hpp`.
- `kite::sendReq` is now a stable virtual protected function in all translation units.
- Added a virtual destructor to `kite` to make the polymorphic test seam safe for base-pointer destruction.
- Existing public REST/ticker methods and DTO signatures were preserved.

## Tests / Verification

```text
/tmp/omnirush/cppkiteconnect-tools/cmake/data/bin/cmake -S . -B /tmp/omnirush/cppkiteconnect-baseline-build -DBUILD_TESTS=ON -DBUILD_EXAMPLES=OFF -DUWS_LIB=/tmp/omnirush/cppkiteconnect-uws-baseline/libuWS.so -DUWS_INCLUDE=/tmp/omnirush/cppkiteconnect-uws-baseline -DGMOCK_ROOT=/tmp/omnirush/cppkiteconnect-gtest-install -DCMAKE_PREFIX_PATH=/tmp/omnirush/cppkiteconnect-gtest-install -DCMAKE_CXX_FLAGS=-I/tmp/omnirush/cppkiteconnect-uws-baseline
/tmp/omnirush/cppkiteconnect-tools/cmake/data/bin/cmake --build /tmp/omnirush/cppkiteconnect-baseline-build -j2
/tmp/omnirush/cppkiteconnect-tools/cmake/data/bin/ctest --test-dir /tmp/omnirush/cppkiteconnect-baseline-build --output-on-failure
/tmp/omnirush/cppkiteconnect-tools/cmake/data/bin/cmake -E chdir /tmp /tmp/omnirush/cppkiteconnect-baseline-build/kiteTest
/tmp/omnirush/cppkiteconnect-tools/cmake/data/bin/cmake -E chdir /tmp /tmp/omnirush/cppkiteconnect-baseline-build/tickerTest
/tmp/omnirush/cppkiteconnect-tools/cmake/data/bin/cmake -S . -B /tmp/omnirush/cppkiteconnect-sanitize-build -DBUILD_TESTS=ON -DBUILD_EXAMPLES=OFF -DUWS_LIB=/tmp/omnirush/cppkiteconnect-uws-baseline/libuWS.so -DUWS_INCLUDE=/tmp/omnirush/cppkiteconnect-uws-baseline -DGMOCK_ROOT=/tmp/omnirush/cppkiteconnect-gtest-install -DCMAKE_PREFIX_PATH=/tmp/omnirush/cppkiteconnect-gtest-install -DCMAKE_CXX_FLAGS="-I/tmp/omnirush/cppkiteconnect-uws-baseline -fsanitize=address,undefined -fno-omit-frame-pointer" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
/tmp/omnirush/cppkiteconnect-tools/cmake/data/bin/cmake --build /tmp/omnirush/cppkiteconnect-sanitize-build -j2
ASAN_OPTIONS=detect_stack_use_after_return=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 /tmp/omnirush/cppkiteconnect-tools/cmake/data/bin/ctest --test-dir /tmp/omnirush/cppkiteconnect-sanitize-build --output-on-failure
```

### Results

```text
Normal C++17 build: PASS.
Normal CTest: 2/2 tests passed; 51 individual GTest cases passed.
External working-directory execution: REST and ticker tests passed from /tmp.
Consumer checks: REST-only, umbrella, ticker-only, standalone headers, and two-TU executables built and ran successfully.
Sanitizer build: compiled successfully. REST test passed.
Sanitizer CTest: ticker-test failed LeakSanitizer with a 1304-byte uWS Group allocation leak from ticker construction.
```

## Header-Only Verification

- No project-owned SDK `.cpp` implementation file or SDK library target was added.
- All SDK changes are in headers.
- Consumer checks compile the headers in REST-only, ticker-only, umbrella, standalone, and multiple-translation-unit configurations.
- Temporary uWS and GoogleTest builds were outside the repository and are not project dependencies or tracked files.

## Problems Discovered

- The legacy uWS `Hub::createGroup` allocation is reported by LeakSanitizer and remains unresolved.
- The repository still lacks package/install/export targets.
- Full malformed binary/CSV and ticker lifecycle coverage is not yet implemented.
- Clang, macOS, Windows, live HTTP/TLS, and live WebSocket verification were not run.

## Decisions Made

- Do not fix the uWS Group ownership leak in Phase 1; it belongs to the planned Phase 2 lifecycle work.
- Do not upgrade dependencies or enable C++20 while the sanitizer gate is open.
- Keep characterization tests for currently observed defects until Phase 2 can invert them to corrected behavior.

## Remaining Work

- Resolve the Phase 1 sanitizer/lifecycle gate.
- Complete malformed-input and callback/lifetime protection coverage.
- Only then begin Phase 2 safety and lifecycle corrections.

## Acceptance Criteria

- [x] C++17 baseline configures and builds with pinned SDK submodules and documented external dependencies.
- [x] Existing offline tests pass from an external build tree.
- [x] Fixture resolution works independently of the process working directory.
- [x] REST-only and ticker/uWS compilation paths are separated.
- [x] The test-only `KITE_UNIT_TEST` class-layout alteration is removed.
- [x] Consumer checks cover umbrella, REST-only, ticker-only, standalone headers, and multiple translation units.
- [x] Protection/characterization tests cover the selected Phase 1 parser, serialization, response, URL, credential, exception, and fixture contracts.
- [ ] Sanitizer test suite passes without the known uWS Group leak.
- [ ] Complete malformed binary/CSV and ticker lifecycle protection suite exists.

## Git Commit

None.

## Notes

Phase 1 is intentionally stopped here. The LeakSanitizer result confirms the planned Phase 2 ownership finding rather than being silently ignored. No Phase 2 implementation has been started.

# Remediation Authorization and Phase 2 Sequencing Decision

The user authorized fixing the blocker and continuing. The known Group leak is
a baseline defect assigned to Phase 2, rather than a new regression caused by
Phase 1. Finish outstanding protection coverage with Phase 2 corrections and
reverify both checkpoints before Phase 3.

Source inspection of uWS v0.14.8 (`Hub.h`, `Hub.cpp`, `Node.h`) establishes that
`Hub::connect` returns no pending-socket handle and provides no TLS verification
configuration. Wrapper-only changes cannot satisfy Phase 2 cancellation and TLS
criteria. The selected Beast/Asio transport work must therefore move forward
from Phase 4 into Phase 2. `plan.md` records the impact; unrelated dependency
upgrades stay in Phase 4. A user-local Ubuntu Boost 1.83 development package is
available for initial verification, without system installation or SDK gitlink
changes. This is an explicitly recorded dependency addition, not an implied
upstream version upgrade.

# Phase 1 — Remediation Checkpoint

## Status
COMPLETED

## Objective
Close the baseline's outstanding protection/leak gates alongside approved Phase 2
corrections, retaining the original failed baseline evidence above.

## Plan Reference
`audit/plan.md`, Phase 1 exit criteria and §7 approved sequencing correction.

## Changes Made
Completed malformed-input/lifecycle/callback protections and added a separate
header-consumer CMake project. Reverified external, repository-child and
arbitrary-cwd execution after replacing the unqualifiable legacy transport.

## Files Changed
`tests/consumer/CMakeLists.txt`, `tests/unit/kite/phase1.cpp`,
`tests/unit/kite/api.cpp`, `tests/unit/tickertest.cpp`,
`tests/unit/ticker_lifecycle.cpp`, `tests/make_test_certificates.py` and Phase 2
headers listed in the next entry; audit/architecture/plan and runtime contract.

## Dependencies Changed
Boost 1.83 added for the approved transport replacement; original SDK gitlinks
unchanged. External GTest/GMock remains 1.10.0.

## API Changes
See Phase 2. No test-only class definition or SDK implementation source introduced.

## Tests / Verification
Commands use CMake/CTest at
`/tmp/omnirush/cppkiteconnect-tools/cmake/data/bin/` (3.28.3).

```sh
cmake -S . -B build/phase2 -DBUILD_TESTS=ON -DBUILD_EXAMPLES=OFF -DBOOST_ROOT=/tmp/omnirush/cppkiteconnect-boost/usr -DGMOCK_ROOT=/tmp/omnirush/cppkiteconnect-gtest-install -DCMAKE_PREFIX_PATH=/tmp/omnirush/cppkiteconnect-gtest-install
cmake --build build/phase2 -j1
ctest --test-dir build/phase2 --output-on-failure
cmake -S tests/consumer -B /tmp/omnirush/cppkiteconnect-phase2-consumer -DBOOST_ROOT=/tmp/omnirush/cppkiteconnect-boost/usr
cmake --build /tmp/omnirush/cppkiteconnect-phase2-consumer -j1
ctest --test-dir /tmp/omnirush/cppkiteconnect-phase2-consumer --output-on-failure
```

### Results
Repository-child CTest PASS 3/3; independent project PASS 5/5. External CTest and
sanitizers PASS 3/3 (details below). From `/tmp/omnirush`, external `kiteTest`
PASS 59/59 and `tickerTest` PASS 4/4; REST/ticker/umbrella/standalone/two-TU
executables ran successfully. Standalone smoke is an aggregate model include
check; exhaustive per-header compilation belongs to Phase 3.

## Header-Only Verification
Production changes remain in headers; independent executables include/link
external dependencies directly without an SDK library. All added `.cpp` files
are tests/consumers. No gitlink changes; `git diff --check` passes.

## Problems Discovered
Concurrent heavy builds exceeded 600000 ms tool timeouts; sequential resumed
builds completed. These were harness/resource failures, not test/compiler failures.

## Decisions Made
Accept this locally verified protection checkpoint; broader platforms/package
consumption are explicitly later gates, not claimed here.

## Remaining Work
Phase 3 packaging and per-header checks; Phase 5 cross-platform C++20 qualification.

## Acceptance Criteria
- [x] External and repository-child offline suites pass.
- [x] Working-directory-independent fixtures and independent consumers pass.
- [x] Selected known defects have corrected regression tests.
- [x] ASan/UBSan and leak gate pass after Phase 2 remediation.
- [x] Missing prerequisites are distinguished from code failures.

## Git Commit
None; changes remain uncommitted.

## Notes
Original Phase 1 PARTIALLY COMPLETED entry remains historical and is not rewritten.

# Phase 2 — Safety, Security and Ticker Lifecycle Stabilization

## Status
COMPLETED

## Objective
Correct confirmed parser/request/auth/error defects and establish tested terminal
shutdown, secure WSS, replay and owner-thread callback behavior on C++17.

## Plan Reference
`audit/plan.md`, Phase 2 and §7 transport-forward authorization.

## Changes Made
- Corrected diagnostic captures, exception override/error mapping, auth refresh,
  SIP modification ID and bracket cancellation parent ID.
- Checked JSON root/envelope/scalar/collection shapes, UTF-8/NUL, CSV range/width,
  candle width/type, bounded binary lengths/counts/recognized packet sizes.
- Initialized required request scalars; rejected incomplete requests before
  transport; preserved indefinite SIP installments `-1`.
- DTO array parsing preserves the DOM and replaces vectors on reparse.
- Added local unsigned percent encoding; retained unused uri-parser gitlink.
- Replaced uWS with inline Beast/Asio session ownership, verified TLS/SNI/hostname,
  bounded queue/messages, connect/write/close deadlines and cancellable retry.
- Added terminal cross-thread stop/status snapshots, explicit ownership rules,
  callback exception containment and subscribe-before-mode replay.
- Preserved peer close codes; normal/policy/auth/TLS failures do not retry.
- Fixed a real stalled-upgrade hang by disarming Beast's internal timeout before
  cancelling/closing TCP. Queued buffers are dropped safely on shutdown.
- Added synthetic TLS/WSS, fragmentation/control frame, callback-copy/reentrancy,
  queue/close, retry exhaustion and REST-backend certificate tests.

## Files Changed
`CMakeLists.txt`, `examples/example{3,4}.cpp`,
`include/kitepp/{exceptions,utils,kite,ticker}.hpp`,
`include/kitepp/kite/{api,internal,gtt,market,mf,order,margins,portfolio}.hpp`,
`include/kitepp/responses/{gtt,margins,market,mf,order,portfolio,ws}.hpp`,
`include/kitepp/ticker/{ws,internal,session}.hpp`,
`tests/unit/kite/{api,mf,phase1}.cpp`, `tests/unit/tickertest.cpp`,
`tests/unit/ticker_lifecycle.cpp`, `tests/make_test_certificates.py`,
`tests/consumer/CMakeLists.txt`, `docs/runtime_contract.md`, `AGENTS.md`,
`audit/{plan,architecture,audit_report,migration_log}.md`.

## Dependencies Changed
Boost 1.83 headers added; uWS/libuv runtime use removed. Temporary Boost provider
is Ubuntu `libboost1.83-dev` 1.83.0-2.1ubuntu3.2 AArch64, extracted outside the
repository. No compiled Boost.System needed in this configuration. OpenSSL
3.0.13, GTest/GMock 1.10.0 and all SDK submodule pins retained. Old CMake/CI
uWS/zlib scaffolding is removed in Phase 3, not mistaken for runtime use.

## API Changes
Additive `tickerOptions` and options constructor; original constructor/callback
signatures retained. Ticker copy/move disabled; stop terminal; setMode requires
prior subscription; owner-thread access enforced during run. Incorrect envelopes,
malformed input and incomplete requests now fail deterministically. See
`docs/runtime_contract.md` for compatibility defaults and destruction constraints.

## Tests / Verification
The following ran with the absolute CMake/CTest 3.28.3 binaries noted above:

```sh
cmake --build /tmp/omnirush/cppkiteconnect-phase2-build -j1
ctest --test-dir /tmp/omnirush/cppkiteconnect-phase2-build --output-on-failure
cmake --build /tmp/omnirush/cppkiteconnect-phase2-asan -j1
ASAN_OPTIONS=detect_stack_use_after_return=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir /tmp/omnirush/cppkiteconnect-phase2-asan --output-on-failure
g++ -fsanitize=thread -x c++ -o /tmp/omnirush/cppkiteconnect-tsan-check - <<< 'int main() { return 0; }'
/tmp/omnirush/cppkiteconnect-tsan-check
git diff --check
git submodule status
```

External normal configuration: BUILD_TESTS/BUILD_EXAMPLES ON, Debug, Boost/GMock
prefixes as above. Sanitizer configuration: tests ON, examples OFF,
`-fsanitize=address,undefined -fno-omit-frame-pointer` compiler flags and matching
link sanitizer flags. Repository-child/independent/cwd commands are above.

### Results
Normal PASS 3/3; all four examples and five consumers built. ASan/UBSan PASS 3/3
with leak and stack-use-after-return checks, including local TLS/lifecycle tests.
Invalid certificates are rejected by REST backend and WSS; replay wire trace
matches subscribe then modes; fragmented binary callback can be copied; pending
upgrade, idle read, backoff and queued-write stop paths drain with bounded close.
No original uWS Group leak remains. Diff whitespace and gitlink identity checks PASS.

NOT RUN — TSan SDK suite: empty process fails `unexpected memory mapping`.
NOT RUN — Clang/macOS/Windows: unavailable in this checkpoint.
NOT RUN — live Kite service: offline synthetic servers only.
NOT RUN — C++20/performance/package qualification: later plan phases.

## Header-Only Verification
New session and all ticker/REST/codec implementations are inline/in-class/template
header definitions. No project-owned SDK `.cpp`, compiled SDK library, pImpl or
runtime introduced. Tests/examples link only external libraries. Single and
multi-TU consumers pass; full package ODR/closure qualification remains Phase 3.

## Problems Discovered
Legacy uWS could not expose cancel/verification controls. Beast TCP cancellation
alone left an internal upgrade timer alive; a failing stalled-upgrade test/strace
identified it, and explicit timeout disarming fixes it. GCC does not accept
Clang's `-fsanitize-address-use-after-return=always`; runtime ASan option used.

## Decisions Made
Select Beast/Asio based on passing security/lifecycle tests; do not retain an
unverified legacy fallback. Preserve scalar defaults/public RapidJSON APIs and
document approximate double conversion; leave internal consuming overload unused
by DTOs rather than introducing leaking allocator ownership. Keep C++17/pins.

## Remaining Work
Phase 3 packaging/macro/provider boundaries; Phase 4 supported dependency updates;
Phase 5 C++20/toolchain matrix. General REST final-wire/date policy, richer errors,
network options and parser-independent models are not claimed complete.

## Acceptance Criteria
- [x] Tested malformed JSON/CSV/candle/text/binary paths reject safely under sanitizers.
- [x] Valid subscription commands and subscribe-before-mode replay verified.
- [x] Terminal stop/cancel/close tests pass across exercised lifecycle states.
- [x] Owner-thread callbacks, borrowed copying and exception containment verified.
- [x] Wrong-host, expired and untrusted WSS certificates rejected.
- [x] Header-only ownership and existing public facade signatures preserved.

## Git Commit
None; changes remain uncommitted.

## Notes
This is a local safety checkpoint, not a final production/cross-platform migration
summary. Future dependency and C++20 changes must rerun the relevant gates.

# Phase 3 — Build-System and Dependency Boundaries

## Status
COMPLETED

## Objective
Provide a genuine relocatable header-only package with tested component closures,
consistent configuration and independent consumers, without upgrading SDK pins.

## Plan Reference
`audit/plan.md`, Phase 3 and acceptance gates 2/9.

## Changes Made
- Replaced global includes/standard/link variables with INTERFACE usage requirements
  and aliases `kitepp::models`, `rest`, `ticker`, `kitepp`.
- Raised CMake floor to 3.18 and verified it. Default top-level build compiles an
  SDK consumer; repository executables explicitly use strict C++17.
- Added bundled-header identity verification, component core/ticker exports,
  package/version config, headers/licenses and relative install requirements.
- Added `config.hpp` and matching transitive TLS/fmt definitions; diagnosed late
  conflicting include configuration. Models are explicitly not transport-independent.
- Added per-header validation TUs and fixed private definition includes revealed
  by the first check; removed an obsolete unscoped Clang diagnostic suppression.
- Extended independent manual/subdirectory/installed and mixed-order multi-TU
  consumers. Added required/optional component discovery checks.
- Removed uWS/libuv/zlib from active SDK build/CI/container requirements. Changed
  bootstrap to system packages; added migration push/PR triggers and pinned tools.
- Added current build guide and historical-setup notices in README/mainpage.

## Files Changed
`CMakeLists.txt`, `cmake/Kitepp{Dependencies,Targets,Install,Tests,HeaderChecks}.cmake`,
`cmake/templates/{kiteppConfig.cmake.in,Doxyfile.in}`, `include/kitepp/config.hpp`,
public entry headers, `utils.hpp`, `kite.hpp`, REST implementation header includes,
`tests/compile/{header_main,ticker_two_tu_a,ticker_two_tu_b,ticker_two_tu_main}.cpp`,
REST two-TU sources, `tests/consumer/{CMakeLists.txt,components/CMakeLists.txt}`,
`.github/workflows/cppkiteconnect-test.yml`, `Dockerfile`, `.dockerignore`,
`docs/{building,mainpage}.md`, `README.md`, `AGENTS.md` and audit records.

## Dependencies Changed
SDK gitlinks unchanged. External package targets OpenSSL/Threads/Boost/GTest replace
legacy discovery/link variables. Tests use GTest/GMock 1.10 config targets (explicit
gtest link also handles its older export metadata). Boost config >=1.83 required
only for ticker. Current pins are checked in `KiteppDependencies.cmake`.
CMake tools 3.18.4.post1, 3.28.3, 4.4.4 installed outside the workspace.
Checkout v7.0.1 commit `3d3c42e5aac5ba805825da76410c181273ba90b1` verified via
official GitHub tag metadata using agent-reach's Jina route; gh lacks authentication.
Docker Ubuntu base digest `534baea6a22c03a63003dbc8dbe78fe34bc0d7e595d9a9dc9834884ff530eb55`.

## API Changes
Additive CMake components and uniform configuration entry point; baseline package
version 2.2.0, not a new release announcement. CMake minimum raised; C++17 retained.
Late incompatible httplib/fmt inclusion now emits a deliberate diagnostic. Manual
inclusion and existing supported facade/DTO entry points remain available.

## Tests / Verification
Commands use binaries under `/tmp/omnirush/cppkiteconnect-{tools,cmake318,cmake444}/cmake/data/bin`.

```sh
cmake -S . -B /tmp/omnirush/cppkiteconnect-phase3-build -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON -DKITEPP_CHECK_HEADERS=ON -DBoost_ROOT=/tmp/omnirush/cppkiteconnect-boost/usr -DCMAKE_PREFIX_PATH=/tmp/omnirush/cppkiteconnect-gtest-install
cmake --build /tmp/omnirush/cppkiteconnect-phase3-build -j1
ctest --test-dir /tmp/omnirush/cppkiteconnect-phase3-build --output-on-failure
cmake --install /tmp/omnirush/cppkiteconnect-phase3-build --prefix /tmp/omnirush/cppkiteconnect-phase3-install
mv /tmp/omnirush/cppkiteconnect-phase3-install /tmp/omnirush/cppkiteconnect-phase3-relocated
```

The same root suite was configured/built using 3.18.4 (examples ON) and 4.4.4 in
separate `phase3-cmake318`/`phase3-cmake444` trees. Old CTest uses `cmake -E chdir
<build> ctest --output-on-failure`; `--test-dir` is not an assumed 3.18 feature.

Independent projects: `tests/consumer`, mode headers/subdirectory with CMake 3.18.4;
for installed mode, copied consumer+compile sources outside the repo, CMake 4.4.4,
relocated prefix, Boost_ROOT and deliberately `CMAKE_CXX_STANDARD=11`. Exported
features correctly produce `-std=c++17`; no SDK source include path appears.
REST-only variants use `KITEPP_CONSUMER_TICKER=OFF` and disabled Boost discovery.
Root REST-only build uses `KITEPP_ENABLE_TICKER=OFF` with header checks and installs
a core-only package. `tests/consumer/components` checks optional/full/unknown and
unavailable required ticker against these packages.

ASan tree `phase2-asan` was reconfigured with the new build, tests ON, original
sanitizer flags, then rebuilt and tested with the recorded ASan/UBSan environment.
Manual `g++ -fsyntax-only` probes verify both late-configuration diagnostics and
the cleaned standalone user header. Workflow YAML/triggers parsed with PyYAML.
Export contents and installed artifact glob inspected; `git diff --check` and
submodule identities checked.

```sh
docker build --tag cppkiteconnect-phase3-check .
docker run --rm --entrypoint /opt/kitepp-build/kitepp-smoke cppkiteconnect-phase3-check
```

### Results
- CMake 3.18.4 / 3.28.3 / 4.4.4 root CTest: PASS 9/9 each.
- Default smoke, all four examples and separate TU for every first-party header built.
- Manual/subdirectory/relocated independent consumers: PASS 6/6 each.
- REST-only subdirectory/installed consumers: PASS 3/3 each, no Boost import/include.
- Optional ticker absent: accepted core; unknown/unavailable required components:
  rejected with package diagnostics. Initial required-ticker probe mistakenly
  listed it as both optional/required; test corrected and actual diagnostic verified.
- Mixed REST/model and REST/ticker include-order multi-TU programs pass.
- ASan/UBSan including leak checks: PASS 9/9.
- Docker AArch64 build/offline smoke: PASS. Built with GCC 13.3, CMake 3.28.3,
  Boost 1.83.0-2.1ubuntu3.2 and OpenSSL 3.0.13-0ubuntu3.16. Base then pinned to the
  exact digest used by that successful build; distro package updates remain intentional.
- Export SDK paths relative; no SDK `.a`/`.so`/`.cpp` installed; no uWS/UV/zlib links.
- Diff whitespace, SDK gitlink identity and workflow YAML checks PASS.

NOT RUN — hosted CI/macOS/Clang/Windows: no hosted run or respective toolchain.
NOT RUN — docs generation: Doxygen absent; documentation release checks are Phase 7.
NOT RUN — TSan: unchanged host runtime mapping failure, recorded in Phase 2.

## Header-Only Verification
All four SDK targets are INTERFACE (including imported exports); SDK files remain
inline/template/in-class headers. Validation TUs are generated only in build trees
and linked into test executables, not an SDK library. Installed artifact inspection
finds no SDK implementation source or binary. External dependencies remain permitted.

## Problems Discovered
First standalone implementation fragments lacked private template definitions;
their includes were corrected rather than suppressing warnings. CTest 3.18 lacks
the assumed modern invocation form, so minimum-version commands use chdir. No
SDK regression or security gate remains open for this local packaging checkpoint.

## Decisions Made
Bundle qualified pinned provider layouts instead of claiming untested arbitrary
external header providers. Resolve only external binary/header config targets
needed by requested components. Keep actual model dependency closure explicit.
System package security updates are allowed; capture qualified identities.

## Remaining Work
Phase 4 independent dependency updates/runtime fmt fix and Phase 5 C++20 matrix.
Phase 7 must consolidate old README/mainpage setup and compile all doc snippets.
Cross-platform/hosted CI results remain unclaimed.

## Acceptance Criteria
- [x] Default build compiles actual SDK headers.
- [x] INTERFACE targets express component dependencies without global SDK flags.
- [x] Relocated installed and add_subdirectory consumers use transitive requirements.
- [x] REST/model consumption avoids ticker/Boost.
- [x] CMake minimum and current lanes agree; sanitizer/multi-TU gates pass.
- [x] No source downloads/mutation or compiled SDK library introduced.

## Git Commit
None; all changes uncommitted.

## Notes
All prior journal entries and audit evidence preserved. The complete migration
has not yet met C++20, platform and release/performance gates.

# Phase 4 — OpenSSL/REST Qualification Checkpoint

## Status
PARTIALLY COMPLETED

## Objective
Qualify one dependency boundary at a time on C++17 before enabling C++20.

## Plan Reference
`audit/plan.md`, Phase 4; wire/TLS/lifecycle acceptance gates.

## Changes Made
Established OpenSSL >=3.0 security-supported-provider/64-bit policy. Added final
wire GET/POST/PUT/DELETE, headers, forms, JSON, error and read-timeout goldens.
Made login URL formatting explicitly runtime with the original fmt 9.1 provider.
Then upgraded cpp-httplib alone to v0.59.0 and added a transport compatibility
adapter preserving original parameter type and final target encoding.

## Files Changed
`include/cpp-httplib` gitlink/checkout; `cmake/KiteppDependencies.cmake`,
`cmake/{KiteppTargets,KiteppInstall}.cmake`, package config template,
`include/kitepp/{config,utils}.hpp`, `include/kitepp/kite/api.hpp`,
`tests/unit/kite/{wire,phase1}.cpp`, `docs/dependencies.md` and this journal.

## Dependencies Changed
- OpenSSL actual provider remains Ubuntu 24.04 `3.0.13-0ubuntu3.16` (verified by
  dpkg-query); minimum 3.0 is explicit, with supported distro backports required.
- cpp-httplib 0.11.4 `7992b148969fbf8093230b37a6a36c2f61135937` → 0.59.0
  `cf3693cb5cc0d39b0e6f4122ba89bda9edb5ac6b`.
- Header SHA256 `dc1e4de3e0ef3a18f3a5818fab563eb3fa45d5d3bb1db527a8d389077ed5ee6c`.
- fmt 9.1, rapidcsv 8.69, GTest 1.10 and Boost 1.83 unchanged at this checkpoint.

## API Changes
Original SDK parameter representation stays `std::multimap<string,string>` rather
than following the provider's new insertion-ordered type. Conversion stays private
to the HTTP send boundary. Target encoding belongs to the SDK; client configuration
disables a second normalization once at construction. Internal helper consumers
with their own httplib client must configure it before use. No public facade/DTO
signature or callback change; documented 64-bit/provider floor now enforced.

## Tests / Verification
Official tag metadata inspected via agent-reach Jina route, then the actual Git
tag fetched and commit verified before checkout. No upstream header edits.

```sh
cmake -S . -B /tmp/omnirush/cppkiteconnect-phase4-build -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON -DBoost_ROOT=/tmp/omnirush/cppkiteconnect-boost/usr -DCMAKE_PREFIX_PATH=/tmp/omnirush/cppkiteconnect-gtest-install
cmake --build /tmp/omnirush/cppkiteconnect-phase4-build -j1
ctest --test-dir /tmp/omnirush/cppkiteconnect-phase4-build --output-on-failure
git -C include/cpp-httplib fetch --depth=1 origin tag v0.59.0
git -C include/cpp-httplib rev-parse v0.59.0
git -C include/cpp-httplib checkout --detach cf3693cb5cc0d39b0e6f4122ba89bda9edb5ac6b
```

Reconfigured/rebuilt the same normal tree after only this dependency change.
Reconfigured/rebuilt `/tmp/omnirush/cppkiteconnect-phase2-asan` with its recorded
sanitizer flags and ran ASAN_OPTIONS/UBSAN_OPTIONS as in prior entries. Absolute
CMake/CTest 3.28.3 binaries used.

### Results
Pre-upgrade wire baseline PASS 9/9 after correcting a mistaken test expectation:
old httplib encodes raw plus as `%2B` (literal plus), not query-space shorthand.
Raw date/time spaces encode `%20`; docs/goldens record actual observed behavior.
This is the historical F31 qualification, not evidence that `%2B` was defective.

First v0.59 run exposed four failures: three mock parameter comparisons became
order-sensitive with the new provider type, and query normalization changed
literal-plus semantics/target bytes. They were corrected at the adapter boundary,
not by weakening expected goldens. Normal PASS 9/9, including 61 REST/protection/
wire cases and unchanged local TLS/lifecycle tests; all four examples build.
ASan/UBSan/leak/multi-TU PASS 9/9. A 1200000 ms combined build timeout interrupted
late sanitizer consumer compilation; resumed build completed and tests passed.

## Header-Only Verification
Adapter/configuration remain inline header functions; SDK targets INTERFACE.
No SDK source/binary introduced. Provider change is an immutable gitlink plus
recorded hash; no first-party edits inside its source tree.

## Problems Discovered
Current httplib's insertion-ordered parameter equality and query normalization
cannot be allowed to silently change SDK contracts. Preserve the original sorted
type and validated legacy encoder; configure clients once to avoid concurrent
option mutation during sends. Raw controls/NUL are not emitted in targets.

## Decisions Made
Keep the verified blocking REST provider; do not use its separate WS implementation
in the already qualified Beast ticker. Preserve the baseline's observed wire
semantics rather than conflating transport upgrade with new datetime/DTO APIs.

## Remaining Work
Independently update/verify fmt, rapidcsv and GTest; then full Phase 4 package/header
and provider checkpoint. C++20 remains disabled in SDK targets.

## Acceptance Criteria
- [x] Actual provider commit/hash verified.
- [x] Isolated pre/post baseline and wire semantics pass.
- [x] Local TLS/lifecycle and relevant sanitizer gates pass.
- [x] Original parameter/signature behavior preserved by adapter.
- [ ] Complete Phase 4 remaining dependency qualification.

## Git Commit
None; all changes uncommitted.

## Notes
This partial entry records the dependency boundary before the next upgrade.
Historical claims and failures remain preserved; no Final Migration Summary yet.

### Dependency checkpoint — fmt 12.2.0

After the cpp-httplib checkpoint passed, fetched/verified tag 12.2.0 and changed
only fmt and its lock: `a33701196adfad74917046096bf5a2aa0ab0bb50` (9.1.0) →
`1be298e1bd68957e4cd352e1f676f00e07dcfb57` (12.2.0). Header hash:
`b95f7c5b45d3d93e7cd8e691ae3040282ebcfe2d9c390f2c19d15ffcfae80a9c`.
No upstream source edits or public signature changes; FMT_HEADER_ONLY retained.
The runtime login format fix was already tested with fmt 9.1 before this update.

Reconfigured/rebuilt `phase4-build` (C++17, examples and tests ON): PASS 9/9,
including unchanged REST wire/error/timeout and local TLS/lifecycle goldens.
Rebuilt ASan targets `kiteTest tickerTest tickerLifecycleTest` and selected
`ctest -R '^(kite-test|ticker-test|ticker-lifecycle-test)$'`: PASS 3/3 with
the same ASan/UBSan/leak environment. Normal single/multi-TU consumers pass;
sanitizer consumer executables were not rerun for this checkpoint. Final Phase 4
package/header/consumer verification remains pending. `git diff --check` passes.

### Dependency checkpoint — rapidcsv 9.07

After fmt passed, fetched/verified v9.07 and changed only rapidcsv and its lock:
`a4877fedd4036f4812263597eec7c84f33e82675` (8.69) →
`cbd8a0a937b249cc07e2db3bfa9cd2cc1689708f` (9.07). Header hash:
`6521f1d20a0cbfc80da767e58c9e071f1b81e6f789f61c7bc0336d3436ad9662`.
No upstream edits or DTO/parser signature changes. Normal C++17 full rebuild:
PASS 9/9, including instrument/MF fixture values, negative row/numeric tests,
wire and TLS/lifecycle goldens. Rebuilt ASan `kiteTest`, selected `^kite-test$`:
PASS 1/1 (61 cases) with leak/UB/stack-use-after-return checks. This focused
sanitizer checkpoint covers the changed CSV path; final all-target verification
is still pending. GTest remains 1.10 until the next independent checkpoint.

### Dependency checkpoint — GTest/GMock 1.18

Fetched/verified v1.18.0 commit `063de7e9578f82b369302001269680b4b1553359` and
created a separate external worktree/build/install under
`/tmp/omnirush/cppkiteconnect-gtest-1.18{,-build,-install}`. The original 1.10
source/install was preserved. Built Release, C++17, extensions OFF, GMock/install
ON. Reconfigured `phase4-build` with explicit new GTest_DIR (avoiding stale cached
provider selection): normal PASS 9/9. No SDK/runtime dependency or source pin for
this test-only provider was introduced. No golden/source adaptation needed.

# Phase 4 — Combined Dependency Acceptance

## Status
COMPLETED

## Objective
Close the isolated dependency and transport/provider checkpoint before C++20.

## Plan Reference
`audit/plan.md`, Phase 4; preceding partial/checkpoint entries retain all failures.

## Changes Made
Qualified the independent updates and their combined SDK consumer/header/wire/
TLS/lifecycle/sanitizer closure. Fixed the minimum-CMake header-check generator
using a configured source template instead of unsupported file(CONFIGURE) content.
Saved a complete qualified C++17 install for comparison/rollback. Added supported
provider/64-bit policy and accurate date/target/parameter compatibility docs.

## Files Changed
See preceding Phase 4 entry, plus `cmake/KiteppHeaderChecks.cmake`,
`cmake/templates/header_check.cpp.in`, `docs/building.md`, `AGENTS.md` and audits.

## Dependencies Changed
- cpp-httplib `cf3693cb5cc0d39b0e6f4122ba89bda9edb5ac6b` (0.59.0).
- fmt `1be298e1bd68957e4cd352e1f676f00e07dcfb57` (12.2.0).
- rapidcsv `cbd8a0a937b249cc07e2db3bfa9cd2cc1689708f` (9.07).
- External GTest/GMock `063de7e9578f82b369302001269680b4b1553359` (1.18.0).
- Supported distro OpenSSL 3.0.13-0ubuntu3.16, Boost 1.83 retained; no SDK linkage
  to uWS/libuv/zlib. RapidJSON/PicoSHA2/fixtures/theme/unused URI gitlinks unchanged.

## API Changes
Existing facade/DTO/callback signatures preserved; original std::multimap parameter
type retained through a private transport adapter. Header/config/setup require
the approved 64-bit/supported-OpenSSL policy. C++17 target contract retained.

## Tests / Verification
Isolated normal/wire/TLS/lifecycle/sanitizer commands/results are above. Combined:

```sh
cmake -S . -B /tmp/omnirush/cppkiteconnect-phase2-asan -DBUILD_TESTS=ON -DBoost_ROOT=/tmp/omnirush/cppkiteconnect-boost/usr -DGTest_DIR=/tmp/omnirush/cppkiteconnect-gtest-1.18-install/lib/cmake/GTest
cmake --build /tmp/omnirush/cppkiteconnect-phase2-asan -j1
ASAN_OPTIONS=detect_stack_use_after_return=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir /tmp/omnirush/cppkiteconnect-phase2-asan --output-on-failure
```

Full CMake 3.18.4 configure/build with tests/examples/header checks ON in
`phase4-build`, new GTest_DIR and recorded Boost_ROOT; CTest via old-tool chdir.
Installed to `/tmp/omnirush/cppkiteconnect-phase4-cxx17-package`. Copied independent
consumer source uses CMake 4.4.4, installed mode, that prefix, Boost_ROOT and
CMAKE_CXX_STANDARD=11; exported cxx_std_17 correctly propagates. Git diff/check/
submodule identities reviewed. Absolute binaries as recorded in prior entries.

### Results
- Combined ASan/UBSan/leak suite PASS 9/9 with GTest 1.18 and all updated providers.
- Combined minimum-CMake full build: all first-party headers/four examples/default
  smoke built, CTest PASS 9/9 (61 REST/protection/wire cases).
- Updated installed single/multi-TU consumers PASS 6/6 under current CMake.
- Provider source clean; only three intended gitlinks changed; diff whitespace PASS.

NOT RUN — hosted CI/macOS/Clang/Windows: respective hosted/toolchain lanes absent.
NOT RUN — live Kite API and production performance: later release qualification.
NOT RUN — TSan: host runtime failure recorded earlier.

## Header-Only Verification
SDK implementation and compatibility adapters remain headers. All four SDK
targets remain INTERFACE; install contains required header/license/config files
and no compiled SDK implementation. Compiled GTest is an external test dependency.

## Problems Discovered
CMake 3.18 file(CONFIGURE) rejects `<` in content; the optional header-check flag
was not exercised in that earlier minimum-version lane. Fixed with configure_file
and now verified with header checks ON. Original wire assumption/provider changes
and resource timeouts are recorded in the preceding checkpoints.

## Decisions Made
Retain tested Beast/Asio; adding a second blocking backend is unnecessary when
the selected adapter passes. Keep JSON/hash/model surfaces stable. Require full
downstream rebuilds for dependency/configuration changes; no ABI promise between
different header/provider builds. Preserve the qualified C++17 checkpoint before
the separate standard change.

## Remaining Work
Phase 5 C++20/compiler/stdlib/platform qualification; explicitly deferred Phase 6
major APIs; Phase 7 measured performance and consolidated release docs/examples.

## Acceptance Criteria
- [x] Every changed dependency has isolated passing baseline/wire/TLS evidence.
- [x] Combined normal/sanitizer/header/package consumer gates pass.
- [x] Supported 64-bit/provider policy documented and enforced where detectable.
- [x] Removed backend libraries do not re-enter SDK target closure.
- [x] Header-only boundary and existing high-level API preserved.

## Git Commit
None; all source/gitlink changes remain uncommitted.

## Notes
The plus-sign test correction preserves the historical audit qualification.
This is still not the complete C++20 migration or a released SDK.

# Phase 5 — C++20 Language and Local Qualification Checkpoint

## Status
PARTIALLY COMPLETED

## Objective
Make C++20 an explicit header-only consumer contract, retain the protected C++17
rollback, and verify behavior/ownership/provider compatibility before release work.

## Plan Reference
`audit/plan.md`, Phase 5 and acceptance gates 1–12. Phase 6 major APIs stay deferred.

## Changes Made
- Default `KITEPP_CXX_STANDARD=20`; only 20/17 accepted. Transitive language feature,
  selected required standard and disabled extensions for repository executables.
- Explicit 17 configuration exports `KITEPP_CPP17_COMPAT=1`. Manual standard
  diagnostics use `_MSVC_LANG` or `__cplusplus`; existing 64-bit policy retained.
- C++20 binary frame/packet views use checked `span<const char>`, integral/non-bool
  constraints, `std::endian`, fixed stack fields and memcpy. C++17 vector decoder
  preserved; no borrowed view escapes into returned tick/depth/callback models.
- Feature-detected CSV floating conversion; libc++ 18 classic-locale fallback
  preserves tested whole-decimal/finite/range rules, including valid subnormals,
  without global locale mutation or a wider intermediate rounding step.
- Regression cases for syntax/overflow/underflow/subnormal suffix/precision and
  global-locale independence. Shutdown test measures stop-to-return and keeps
  its peer stalled through setup; one-second assertion retained.
- Opt-in offline decoder allocation/latency benchmark with permanent private
  validation friendship, not a macro-dependent ticker definition.
- CI explicitly selects 20/17 and compiler/stdlib; libc++ GTest is built against
  matching stdlib. Build/runtime/dependency guidance documents full downstream
  rebuilds and actual qualified versus unrun toolchains.

## Files Changed
`CMakeLists.txt`, `cmake/KiteppTargets.cmake`, `cmake/KiteppTests.cmake`,
`cmake/KiteppHeaderChecks.cmake`, `include/kitepp/config.hpp`,
`include/kitepp/utils.hpp`, `include/kitepp/ticker/ws.hpp`,
`include/kitepp/ticker/internal.hpp`, `tests/unit/kite/phase1.cpp`,
`tests/unit/ticker_lifecycle.cpp`, `tests/consumer/CMakeLists.txt`,
`tests/benchmarks/decoder.cpp`, `.github/workflows/cppkiteconnect-test.yml`,
`docs/building.md`, `docs/runtime_contract.md`, `docs/dependencies.md`,
`README.md`, `docs/mainpage.md`, `AGENTS.md`, and audit updates.

## Dependencies Changed
No further SDK gitlink/provider upgrade. Phase 4 locks and supported Ubuntu
OpenSSL/Boost 1.83 remain. Added external verification tools only: Clang 18.1.3,
compiler-rt and libc++/libc++abi 18 in Docker image
`cppkiteconnect-phase5-clang-check` (local image SHA
`a3aff57c91540aa88da593da229908a98e455882f2fbd7cd5281375f6c77b8e6`).
External GTest 1.18 was separately rebuilt with Clang/libc++/C++20 for that lane.
This is not a new SDK binary/runtime or an immutable upstream provider claim.

## API Changes
Default language contract changes to C++20, with explicit C++17 rollback. Existing
REST/ticker/DTO/callback exception-facing surfaces retained. Only private decoder
views and benchmark validation access change; public return/callback data remain
owning. Existing Phase 4 runtime fmt fix qualified under the new standard.

## Tests / Verification
Absolute tools and external providers are those recorded in preceding entries:
CMake 4.4.4 at `/tmp/omnirush/cppkiteconnect-cmake444/cmake/data/bin`, CMake 3.18.4
at `/tmp/omnirush/cppkiteconnect-cmake318/cmake/data/bin`, GCC 13.3,
`Boost_ROOT=/tmp/omnirush/cppkiteconnect-boost/usr`,
`GTest_DIR=/tmp/omnirush/cppkiteconnect-gtest-1.18-install/lib/cmake/GTest`.

Normal GCC C++20 `phase5-build`: BUILD_TESTS/BUILD_EXAMPLES/KITEPP_CHECK_HEADERS ON;
full header/example build and CTest 9/9. After final CSV/test additions, rebuilt
`kiteTest tickerLifecycleTest` and reran all nine groups: PASS (62 REST/protection/
wire, four binary/text, 19 TLS/lifecycle cases plus consumer programs).

```sh
cmake -S . -B /tmp/omnirush/cppkiteconnect-phase5-cxx17 -DKITEPP_CXX_STANDARD=17 -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON -DKITEPP_CHECK_HEADERS=ON -DBoost_ROOT=/tmp/omnirush/cppkiteconnect-boost/usr -DGTest_DIR=/tmp/omnirush/cppkiteconnect-gtest-1.18-install/lib/cmake/GTest
cmake --build /tmp/omnirush/cppkiteconnect-phase5-cxx17 --parallel 1
cmake -E chdir /tmp/omnirush/cppkiteconnect-phase5-cxx17 ctest --output-on-failure

cmake -S . -B /tmp/omnirush/cppkiteconnect-phase5-asan -DBUILD_TESTS=ON -DBoost_ROOT=/tmp/omnirush/cppkiteconnect-boost/usr -DGTest_DIR=/tmp/omnirush/cppkiteconnect-gtest-1.18-install/lib/cmake/GTest -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build /tmp/omnirush/cppkiteconnect-phase5-asan --parallel 1
ASAN_OPTIONS=detect_stack_use_after_return=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir /tmp/omnirush/cppkiteconnect-phase5-asan --output-on-failure
```

Rollback full build used CMake 3.18.4; ASan used 4.4.4. Interrupted commands were
resumed, then tests run only after builds completed. GCC sanitizer initial result
8/9, with shutdown timing failure described below; final result 9/9 after test fix.

Container source `/sdk` is a read-only mount of the repository; `/build` maps to
`/tmp/omnirush/cppkiteconnect-phase5-clang`. Clang normal/libstdc++ `normal` full
headers/examples/tests passed 9/9 at initial C++20 checkpoint (61 REST cases).
Current Clang `asan` uses system libstdc++, matching external GTest, BUILD_TESTS ON,
flags `-fsanitize=address,undefined -fno-omit-frame-pointer
-fsanitize-address-use-after-return=always`, matching linker flags; full 9/9 PASS
with ASAN_OPTIONS/UBSAN_OPTIONS above (62 REST cases). Compiler-rt installed.

For libc++, external GTest source mount `/gtest-source` was configured to
`/build/libcxx-gtest`, `CMAKE_CXX_COMPILER=clang++-18`,
`CMAKE_CXX_FLAGS=-stdlib=libc++`, `CMAKE_CXX_STANDARD=20`, installed to
`/build/libcxx-gtest-install`. SDK `/build/libcxx` uses that GTest_DIR and compiler/
stdlib, BUILD_TESTS/BUILD_EXAMPLES/KITEPP_CHECK_HEADERS ON. Full build and CTest
PASS 9/9; final changed lifecycle target rebuilt, all groups rerun: PASS 9/9.

Minimum CMake 3.18.4 C++20 default `phase5-minimum` configure/smoke build/run PASS.
Installed C++20 to `phase5-package`, renamed to `phase5-relocated`. Independent
source copy `/tmp/omnirush/cppkiteconnect-package-check/consumer` used installed
mode under CMake 3.18.4, that prefix, Boost_ROOT and `CMAKE_CXX_STANDARD=11`.
Built actual `-std=c++2a` via exported cxx_std_20; CTest PASS 6/6. Exports contain
relative SDK paths/external targets, no build-machine paths or SDK binary.

Current-source independent manual and subdirectory consumers in
`phase5-manual`/`phase5-subdirectory` passed 6/6 each under CMake 4.4.4 (subdirectory
requests C++11 and receives C++20). Full-package REST/model-only `phase5-rest-installed`
passed 3/3, with Boost find disabled; unused-disable warning confirms no discovery.
C++17 `phase5-cxx17-package` exports cxx_std_17 and compatibility macro;
`phase5-cxx17-consumer`, older requested C++11, PASS 6/6 under current CMake.
The original Phase 4 C++17 install remains separately preserved.

Six compiler-input diagnostics PASS: default C++20, default C++17 rejected,
explicit C++17 accepted, C++14 rollback rejected, late fmt rejected, late httplib
rejected. Workflow YAML/triggers/six matrix lanes and every substituted shell
script parsed successfully. SDK INTERFACE definitions, first-party compiled-source
absence, tracked/untracked edits, gitlink identities and diff whitespace reviewed.

### Decoder measurement

Release `phase5-bench17`/`phase5-bench20`: KITEPP_BUILD_BENCHMARKS ON, respective
KITEPP_CXX_STANDARD, GCC 13.3 `-O3 -DNDEBUG`, same provider/host. Target
`kitepp-decoder-benchmark`; 100 warmups, 5000 samples, synthetic 184-byte full
packets. All checksums PASS. Final observed sample after compiler work completed:

| Standard | Packets/frame | Allocations/frame | Mean ns | p50 ns | p99 ns | Checksum |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| C++17 | 1 | 62 | 4194 | 1720 | 1800 | 5000 |
| C++17 | 16 | 940 | 29721 | 27000 | 54160 | 80000 |
| C++17 | 64 | 3728 | 118409 | 107000 | 251240 | 320000 |
| C++20 | 1 | 12 | 1004 | 520 | 520 | 5000 |
| C++20 | 16 | 170 | 8968 | 8160 | 11800 | 80000 |
| C++20 | 64 | 654 | 33973 | 30480 | 39160 | 320000 |

Counts are ordinary new/new[] calls during decode/checksum/returned-model lifetime;
they exclude aligned allocation/direct malloc/provider-internal allocations.
p50/p99 include decode/checksum timing before tick destruction; mean also includes
destruction/loop/timer overhead. Shared AArch64 host is not isolated: means/tails
varied in earlier runs (some concurrent with compilation), while allocation counts
were stable. This supports reduced temporary allocations only, not a production
latency/SLA claim. Further reservations/moves/HTTP tuning belong to Phase 7.

### Results
- Local current GCC C++20 normal and GCC/Clang ASan/UBSan/leak groups PASS 9/9.
- Clang/libc++ headers/examples/current offline groups PASS 9/9.
- Explicit C++17 minimum-tool header/example/offline groups PASS 9/9.
- Manual/subdirectory/relocated and rollback installed consumers PASS 6/6 each;
  installed REST/model-only PASS 3/3. Minimum/current-tool consumption agrees.
- Language/configuration diagnostics and workflow syntax PASS; header-only closure
  retained. Synthetic checksums/allocations verified in both language modes.

NOT RUN — macOS AppleClang/libc++, Windows, Linux x86_64 and other compiler/provider
versions: respective runners/toolchains absent. Hosted CI (including new combined
libc++ sanitizer lane) is not evidenced by workflow syntax. NOT RUN — TSan: prior
host mapping failure. NOT RUN — Doxygen: absent. NOT RUN — live Kite/downstream
application/production performance: requires separate qualification.

## Header-Only Verification
All SDK implementation remains inline/in-class/template headers. All four SDK
targets remain INTERFACE; no SDK `.cpp`, object archive, shared binary or pImpl
introduced. Generated validation sources, benchmarks and compiled external GTest
are test/build tooling, not SDK implementation. Same class definition is used by
tests/consumers; no test-only macro changes its layout or virtual methods.

## Problems Discovered
- libc++ 18 deletes/rejects the double from_chars call. Fixed by overload detection
  and the classic-locale fallback. Wider intermediate parsing was rejected to
  avoid double rounding; a precision regression checks direct target conversion.
- Initial sanitizer shutdown assertion measured TLS/1000 subscriptions plus stop;
  observed 1.249 s versus 1 s and reproduced 1.218/1.170 s. Timer now begins at stop
  and peer remains stalled through preparation. Final GCC/Clang sanitizer and
  normal/rollback lifecycle suites pass; no SDK shutdown deadline relaxed.
- Concurrent heavy builds exceeded 1200000 ms; GCC resumed with 1800000 ms.
  Docker compilation continued after client timeout, completed and logs showed
  all tests passing; later final lifecycle rerun returned explicit PASS output.
- An initial focused filter matched zero tests; not counted as verification.
  Correct `kiteProtection.shortCSVAndCandlesAreRejected` then ran/passed under libc++.
- Initial GCC release reverse-copy warning removed with bounded fixed-size loops;
  final benchmark builds are warning-free in observed output.

## Decisions Made
Keep the qualified C++17 configuration until the complete C++20 matrix is accepted;
all downstream TUs must be rebuilt consistently. C++20 changes remain private and
measurable; no JSON replacement, async REST, major models, result API or C++23
dependency added. Report Linux qualification precisely; absent platforms are not
inferred from compiler success or a CI file. Do not advance gated release work
or write a Final Migration Summary while Phase 5 platform evidence is outstanding.

## Remaining Work
Execute remaining declared compiler/OS lanes, especially macOS AppleClang/libc++
and hosted CI with recorded providers; qualify the complete supported matrix.
After acceptance, Phase 7 consolidated docs/snippet compilation/token-output
cleanup, further measured performance and release/install checks. Phase 6 major
model/result interfaces remain explicitly deferred.

## Acceptance Criteria
- [x] Explicit C++20 targets/strict repository executables and protected C++17 rollback.
- [x] Useful checked span/endian/constrained decoding with owning callback compatibility.
- [x] Available Linux headers/examples/offline/wire/TLS/lifecycle/sanitizer checkpoints.
- [x] Installed/minimum/current-tool and single/multi-TU consumers; rebuild guidance.
- [x] Measured synthetic allocation improvement with honest latency limits.
- [x] Header-only boundary and configuration/provider consistency reviewed.
- [ ] Complete oldest/current supported compiler/OS matrix and downstream acceptance.

## Git Commit
None; all migration source/gitlink changes remain uncommitted.

## Notes
This is a local implementation/qualification checkpoint, not complete Phase 5
platform acceptance, a final migration summary or a released SDK.

### Final review follow-up

After recording the checkpoint, the corrected shutdown test passed three repeated
GCC ASan/UBSan/UAR runs. Total test times 1.062/1.382/1.374 s include setup and the
server script; the unchanged stop-to-return assertion passed each time. Rollback
installed compiler flags explicitly show `-std=c++17` and the compatibility macro
in all six programs. Final workflow/root-CMake diff and whitespace review PASS.
Added the offline probe command to the build guide; no SDK source changed after
the recorded verification.

# Phase 5 — Newer Toolchains, libc++ Sanitizers and Optional Components

## Status
PARTIALLY COMPLETED

## Objective
Continue the remaining locally feasible compiler/stdlib and installed-consumer
qualification, investigate failures, and preserve the outstanding platform gate.

## Plan Reference
Phase 5, acceptance gates 2/9/10/11; package dependency boundaries from Phase 3.

## Changes Made
- Qualified newer supported Ubuntu compiler packages and the actual CI GTest 1.14
  provider, independently of the previously qualified external 1.18 build.
- Isolated the libc++ 18 sanitizer allocator failure in a no-SDK nested-exception
  probe. Qualified LLVM 20 as the replacement libc++ sanitizer provider without
  disabling mismatch checks or changing exception-facing SDK behavior.
- Corrected installed optional ticker discovery: optional missing Boost no longer
  rejects required core components. Required streaming still uses find_dependency;
  optional streaming uses quiet find_package and conditional target import.
- Expanded CI to eight explicit language/provider lanes, retained 18 normal
  compatibility, added GCC 14/LLVM 20 and the optional-Boost consumer regression.
- Updated actual provider/matrix documentation and repository guidance.

## Files Changed
`cmake/templates/kiteppConfig.cmake.in`, `.github/workflows/cppkiteconnect-test.yml`,
`docs/building.md`, `docs/dependencies.md`, `AGENTS.md` and audit updates.
Verification-only Dockerfiles/probe/builds under `/tmp/omnirush` are outside the
repository. No first-party implementation header or bundled provider source edited.

## Dependencies Changed
No SDK gitlink upgrade. External qualification images:
- `cppkiteconnect-phase5-ci-provider`, image
  `f647a2b08317dbc3600b2964ba93a7addbd91c47a8de7d6b5be558300581b572`:
  Clang/libc++/libc++abi 18.1.3-1ubuntu1 plus GTest/GMock 1.14.0-1.
- `cppkiteconnect-phase5-newer-tools`, image
  `e9591b8d01c7594b8029e172704c567a46ceb7dcd54e452cabbe9b6b5d188041`:
  g++ 14.2.0-4ubuntu2~24.04.1, Clang/compiler-rt/libc++/libc++abi
  20.1.2-0ubuntu1~24.04.3, GTest/GMock 1.14.0-1.
- OpenSSL dev remains 3.0.13-0ubuntu3.16; Boost 1.83.0-2.1ubuntu3.2.
Images use previously recorded Ubuntu bootstrap; apt updates are external tool
provisioning, not an SDK dependency update or immutable upstream latest-version
claim. These are available newer distro providers, not every current compiler.

## API Changes
Installed optional-component behavior corrected; existing required component and
language/ownership/API contracts retained. No strong-model/result API work begun.

## Tests / Verification
Native Linux/AArch64 Docker with `/sdk` read-only source and `/build` bind-mounted
to `/tmp/omnirush/cppkiteconnect-phase5-clang`. Container CMake 3.28.3. Host minimum
and current CMake absolute tools remain those recorded above.

### Newer compiler checkpoints

`/build/gcc14`: CMAKE_CXX_COMPILER=g++-14, KITEPP_CXX_STANDARD=20,
BUILD_TESTS/BUILD_EXAMPLES/KITEPP_CHECK_HEADERS ON; distro GTest discovered.
Configured/built --parallel 1, CTest PASS 9/9. Installed to gcc14-install, renamed
gcc14-relocated; tests/consumer installed mode with that prefix, g++-14 and
CMAKE_CXX_STANDARD=11 built and passed 6/6 (actual interface raises the standard).

Matching GTest builds `ci-gtest18`/`ci-gtest20` use source `/usr/src/googletest`,
respective clang++-18/20, CMAKE_CXX_STANDARD=20, CMAKE_CXX_EXTENSIONS=OFF,
CMAKE_CXX_FLAGS=-stdlib=libc++; installed to respective `ci-gtest*-install`.
Both external test-provider builds pass.

`/build/clang20-libcxx`: clang++-20, -stdlib=libc++, matching GTest_DIR,
KITEPP_CXX_STANDARD=20, tests/examples/header checks ON. Full build and CTest PASS
9/9 (62 REST/protection/wire, four binary/text, 19 TLS/lifecycle cases plus consumers).

```sh
cmake -S /sdk -B /build/clang20-libcxx-asan -DCMAKE_CXX_COMPILER=clang++-20 -DKITEPP_CXX_STANDARD=20 -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON -DGTest_DIR=/build/ci-gtest20-install/lib/cmake/GTest -DCMAKE_CXX_FLAGS="-stdlib=libc++ -fsanitize=address,undefined -fno-omit-frame-pointer" -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++ -fsanitize=address,undefined"
cmake --build /build/clang20-libcxx-asan --parallel 1
ASAN_OPTIONS=detect_stack_use_after_return=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir /build/clang20-libcxx-asan --output-on-failure
```

PASS 9/9 including exception containment, borrowed payloads, malformed fields,
TLS rejection and terminal cancellation. Normal lane already compiled all headers;
sanitizer lane builds examples/tests/single/multi-TU consumers, header flag OFF.
Installed to clang20-install, renamed clang20-relocated; independent installed
consumer requests C++11, uses clang++-20/-stdlib=libc++, PASS 6/6. Optional ticker
without Boost configured successfully from the same relocated install.

### Failed libc++ 18 sanitizer provider and isolation

`/build/ci-libcxx18-asan` uses clang++-18, matching GTest 1.14, same stdlib/sanitizer
flags/environment, tests/examples/header checks ON. Full build completed (Docker
continued after 1800000 ms client timeout), CTest **FAIL 8/9**. Lifecycle aborts
at callbackExceptionsStopWithoutRecursiveErrors with operator-new/free mismatch,
39-byte allocation from libc++.so.1 and destruction in libc++abi.so.1. The planned
install/consumer portion did not run after this failure; not counted as passing.

Standalone `/build/exception_probe.cpp` (no SDK, Boost, TLS or GTest):

```cpp
#include <stdexcept>
int main() {
    try { throw std::runtime_error("callback"); }
    catch (const std::exception&) {
        try { throw std::runtime_error("error callback"); }
        catch (const std::exception&) {}
    }
}
```

Compiled/run with clang++-18 -std=c++20 -stdlib=libc++ -fsanitize=address,undefined
-fno-omit-frame-pointer and the same ASAN_OPTIONS/UBSAN_OPTIONS: reproduces identical
39-byte allocation mismatch, exits nonzero. dpkg reports matching libc++/libc++abi
18.1.3-1ubuntu1 packages, not a GTest ABI mismatch. addr2line locates constructor
in runtime_error and destruction/__cxa_end_catch in libc++abi. Symbolizer absence
is recorded but does not change the allocation diagnostic.

The identical probe with clang++-20/LLVM 20 provider and the same checks exits zero.
Full unsuppressed LLVM 20 SDK sanitizer PASS above closes the available sanitizer
lane. No alloc_dealloc_mismatch=0, test removal, exception-swallowing change or
first-party allocation workaround was used. 18 normal compatibility evidence
remains historical/current normal evidence, not successful sanitizer qualification.

### Optional-component regression

Actual full C++20 package initially failed:

```sh
cmake -S tests/consumer/components -B /tmp/omnirush/cppkiteconnect-phase5-optional-no-boost -DCMAKE_PREFIX_PATH=/tmp/omnirush/cppkiteconnect-phase5-relocated -DKITEPP_EXPECT_TICKER=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Boost=ON
```

find_dependency inherited package-level REQUIRED for the optional ticker's Boost,
rejecting the required rest component. Earlier optional test only covered a
ticker-disabled install, so it did not characterize this full-package case.
After config fix, reconfigured producer and refreshed install:
- CMake 4.4.4 `phase5-optional-no-boost`: PASS; CMake 3.18.4 corresponding
  `phase5-optional-no-boost318`: PASS.
- Full optional ticker with recorded Boost_ROOT: PASS.
- Required ticker/umbrella without Boost and unknown required component: expected
  failures confirmed with nonzero exit and intended REQUIRED/unknown diagnostics.
- Updated normal installed consumers: configure and PASS 6/6.
- Updated C++17 rollback package: CMake 3.18 optional missing Boost PASS; installed
  consumer reconfigure and CTest PASS 6/6 with existing verified compiler/macro flags.
- Clang 20 relocated package optional missing Boost: PASS.
New CI regression uses this same independent component consumer after relocation.

Workflow YAML, push/PR/dispatch triggers, all eight matrix rows and every substituted
shell script syntax PASS. SDK implementation boundary and current source/config/
provider diffs reviewed; no source pin or SDK archive/shared binary introduced.

## Results
- New GCC 14 and Clang/libc++ 20 full normal checkpoints PASS 9/9 each.
- Qualified Clang/libc++ 20 ASan/UBSan/leak checkpoint PASS 9/9.
- New compiler relocated consumers PASS 6/6 each; rollback consumers remain 6/6.
- Optional-Boost regression fixed, verified with minimum/current CMake and both
  language packages; required failures retain intended behavior.
- libc++ 18 sanitizer failure classified by standalone repro; not falsely passed.

NOT RUN — hosted CI/macOS/Windows/native x86_64: gh has no authenticated hosts,
no macOS runner is available and binfmt_misc has no x86_64 interpreter. No remote
workflow was dispatched or source committed/pushed. NOT RUN — TSan: previously
recorded host mapping failure. Live service/release/production performance remain
separate pending work. Compiler checks on this machine do not qualify other OSes.

## Header-Only Verification
First-party SDK still has only header implementation and four INTERFACE targets.
Only installed config selection changed. External toolchains/test archives and
the standalone probe are qualification tools, not SDK implementation/runtime.

## Problems Discovered
The actual optional-Boost case and libc++ 18 runtime sanitizer failure are retained
above with their original outcomes. Long client timeout did not terminate Docker;
wait/log evidence established completed build and exit 8, not timeout-based success.
Newer compiler normal and qualified sanitizer runs returned explicit PASS output.

## Decisions Made
Select a provider that passes unsuppressed sanitizer qualification; retain the
older normal compatibility lane with accurate limitations. Distinguish optional
component dependency absence from required dependency failure. Keep original
platform gate and C++17 rollback until macOS/remaining matrix evidence or an
explicit user-approved sequencing/scope adjustment is obtained.

## Remaining Work
Authenticated hosted/native platform execution, especially macOS AppleClang/libc++.
Phase 7 work is still gated; no final migration summary/release acceptance yet.

## Acceptance Criteria
- [x] Newer available compiler/stdlib/provider checkpoints and installed consumers.
- [x] Unsuppressed libc++ sanitizer lane, with matching standard-library GTest.
- [x] Actual optional dependency absence regression and required failure checks.
- [x] Header-only boundary/source pins, accurate CI/docs and original failure record.
- [ ] Remaining declared compiler/OS matrix and downstream acceptance.

## Git Commit
None; migration working tree remains uncommitted.

## Notes
This continues local Phase 5 evidence. It does not waive the outstanding platform
gate or silently authorize advancing later phases.

# Phase 7 — Local Release and Documentation Qualification

## Status
COMPLETED for the available Linux/AArch64 release checkpoint; platform release
acceptance remains pending.

## Objective
Consolidate current documentation, remove obsolete runtime guidance and credential
output, compile documented examples, generate API documentation, rerun package and
benchmark checks, and record release-facing limitations.

## Changes Made
- Replaced README and Doxygen mainpage with current C++20/64-bit/Beast-Asio,
  CMake package, TLS, callback, shutdown, dependency and compatibility guidance.
- Added `docs/release-notes.md`; installed it with the package. It documents
  migration behavior, dependency baseline, deferred APIs and downstream rebuild/
  secret-handling checklist without claiming a new package release.
- Removed stale uWS build commands and C++17-default claims from user-facing docs.
- Added Graphviz/Doxygen documentation inputs and CI documentation job. Doxygen
  now includes building, dependency, runtime and release pages.
- Added `tests/compile/documented_examples.cpp` and the
  `kitepp-documented-examples` target. It validates the README REST/ticker code
  without contacting the service.
- Examples 1/3/4 now validate required environment variables; example1 no longer
  prints the generated access token. Example2's commented guidance says to store
  the token securely rather than print it.

## Files Changed
`README.md`, `docs/mainpage.md`, `docs/release-notes.md`, `docs/building.md`,
`cmake/templates/Doxyfile.in`, `cmake/KiteppInstall.cmake`, `CMakeLists.txt`,
`.github/workflows/cppkiteconnect-test.yml`, `examples/example1.cpp`,
`examples/example2.cpp`, `examples/example3.cpp`, `examples/example4.cpp`,
`include/kitepp/ticker/ws.hpp`, and `tests/compile/documented_examples.cpp`.

## Dependencies Changed
No SDK/provider revision changed. Documentation validation used the existing
Ubuntu container with Doxygen 1.9.8 and Graphviz 2.42.2. The package still carries
the Phase 4 locked providers and qualified OpenSSL/Boost policy.

## API Changes
No public API, DTO, callback or exception surface was changed. The additive
documented-example target and installed release notes are build/package tooling.
Examples now fail early with status 2 when required credentials are absent and no
longer display generated access tokens.

## Tests / Verification
Phase7 C++20 tree `/tmp/omnirush/cppkiteconnect-phase7-build`: configured with
CMake 4.4.4, GCC 13.3, Boost_ROOT recorded provider, GTest 1.18, tests/examples/
header checks/benchmark ON. Build PASS, CTest PASS 9/9, documented-example target
PASS. Four live examples compile; example 1/3/4 missing-env smoke returns 2 and
example2 exits 0 without service access.

Phase7 C++17 tree `/tmp/omnirush/cppkiteconnect-phase7-cxx17`: explicit
KITEPP_CXX_STANDARD=17, examples/header checks ON; all targets compile, including
the documented-example check. Manual syntax probes with Boost include path pass for
C++20 and C++17 plus `KITEPP_CPP17_COMPAT=1`.

Decoder probes use Release CMake 4.4.4, GCC 13.3, same host/provider, 100 warmups,
5000 iterations and checksums. C++17 `/tmp/omnirush/cppkiteconnect-phase7-bench17`:
allocations/frame 62/940/3728 for 1/16/64 packets; checksums 5000/80000/320000.
C++20 phase7 tree: 12/170/654; same checksums. Timing samples vary materially on
the shared host and are not release latency evidence.

Installed C++20 phase7 package `/tmp/omnirush/cppkiteconnect-phase7-package`
contains `share/kitepp/release-notes.md`; independent relocated consumer under
CMake 4.4.4, requested C++11 and Boost_ROOT, CTest PASS 6/6. Existing optional
Boost/REST-only package and C++17 rollback package gates remain recorded above.

Doxygen command ran in a read-only `/sdk` source container with Doxygen 1.9.8 and
Graphviz 2.42.2; output includes `docs/html/index.html` and the release-notes page.
Target returned success. Legacy public header comments emit non-fatal repeated
`ex1` paragraph/stale-parameter warnings; undocumented-member warnings are disabled
in the Doxygen configuration. These comments remain future documentation debt.

CI YAML/triggers/shell syntax was revalidated after adding the documentation job.
`git diff --check` passes. No live credentials/service calls, release artifact
upload, commit or push.

## Results
- [x] Current README and Doxygen mainpage replace stale uWS/C++17 instructions.
- [x] Release notes/downstream checklist installed with the package.
- [x] Examples and documented REST/ticker snippets compile in C++20 and rollback.
- [x] Examples avoid access-token output and reject absent credentials safely.
- [x] Doxygen + Graphviz HTML generation passes; warnings documented.
- [x] C++20/C++17 benchmark checksums and allocation comparisons rerun.
- [x] Relocated package consumer and package-installed documentation verified.
- [ ] Complete macOS/Windows/other-platform release matrix and live-service signoff.

## Header-Only Verification
No SDK implementation source or compiled SDK library was introduced. The
documented-example executable, generated documentation and benchmark remain
validation tooling. All SDK targets continue to be INTERFACE.

## Problems Discovered
Doxygen's legacy public comments use repeated `ex1` paragraph labels and contain
stale parameter names; these generate warnings but do not prevent HTML generation.
One attempted host-only header probe lacked the external Boost include path; the
corrected probe with the recorded Boost include path passed in both language modes.

## Decisions Made
Complete the local release/documentation checkpoint without claiming a cross-platform
release. Keep API modernization deferred to Phase 6 and retain honest benchmark and
live-service limits. Keep generated docs out of source control.

## Remaining Work
Hosted macOS/Windows/other platform execution, live-service qualification and final
release approval/artifact publication. Phase 7 local work is complete; these gates
remain required before calling the SDK broadly released.

## Acceptance Criteria
- [x] Documentation matches current C++20/Beast/Asio implementation.
- [x] All repository/live examples and documented snippets compile.
- [x] Credential output removed and missing-env behavior checked.
- [x] Doxygen/Graphviz generated HTML and installed release notes verified.
- [x] Offline package/consumer/benchmark checks rerun.
- [ ] Complete platform/live-service release acceptance.

## Git Commit
None; all migration and release-documentation changes remain uncommitted.

## Notes
This is the completed local Phase 7 checkpoint, not a release upload or final
migration summary while the declared platform gate remains open.
