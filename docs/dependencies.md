# Dependency policy and qualification

The modernized SDK targets 64-bit platforms. SDK header revisions/hashes are
recorded in `cmake/KiteppDependencies.cmake`; configuration is read-only and never
downloads or updates providers. Test fixtures and documentation theme retain
their recorded gitlinks unless explicitly upgraded and verified.

Use **OpenSSL >=3.0 from a security-supported provider**. A supported distro's
backported 3.0 package is acceptable; unsupported bare upstream 3.0 builds are
not the production policy. Prefer maintained 3.5 LTS where available. The current
Linux checkpoint uses Ubuntu 24.04 `3.0.13-0ubuntu3.16`, including package-provided
trust roots; local REST/WSS certificate tests exercise verification. CMake can
check the version, not a provider's support lifecycle. Resolve supported patches
through the provider rather than shipping a private OpenSSL build.

Beast/Asio uses package-provided Boost config targets >=1.83. Version 1.83 is the
tested floor; later versions are not qualified merely by satisfying version
discovery. Peer-chain/hostname/SNI validation and cancellable lifecycle behavior
are acceptance requirements, not inferred from a backend name. The passing
primary adapter eliminates a need to add a second unverified WebSocket backend.

cpp-httplib remains blocking REST. fmt remains header-only, with explicit runtime
format strings. CSV keeps rapidcsv and validated numeric/row boundaries. RapidJSON
and PicoSHA2 stay pinned: public parser compatibility and session checksum goldens
are retained. GTest/GMock are external test-only packages, not SDK dependencies.
uWS/libuv/zlib are absent from the SDK's active dependency closure. A build tool
such as CMake may itself use libuv; that is not a ticker link requirement.

C++20 is the default language contract, with an explicit C++17 rollback mode.
See [building.md](building.md) for the qualified compiler/standard-library matrix
and full downstream rebuild requirements. When testing with libc++, build GTest
against the same standard library instead of linking a system libstdc++ archive.
Ubuntu's LLVM 20.1.2 libc++/libc++abi provider is qualified for ASan/UBSan;
the 18.1.3-1ubuntu1 runtime fails a standalone nested-exception allocator check.
Normal compiler checks do not override that failed sanitizer qualification.
GTest/GMock 1.14.0-1 is also qualified as the Ubuntu CI test provider.

Dependency candidates must be updated one at a time and pass the C++17 baseline,
wire goldens, local TLS/lifecycle and applicable sanitizers before proceeding.
Exact provider commits and command results belong in the append-only migration
journal. Historical audit latest-release observations are candidates, not locks.

Historical date/time fields are strings interpolated into query targets. Supply
raw spaces between date/time, not `+` as a space substitute: the SDK adapter
correctly encodes a raw **literal** plus as `%2B`, while spaces become `%20`.
Already percent-encoded components retain their spelling in the qualified wire
test. Quote symbols and WSS credentials are encoded as raw components by the SDK.
This is not a new typed datetime API and does not automatically normalize all
REST path inputs or validate datetime syntax.

The REST adapter owns final target encoding and disables a second provider
normalization step. Its parameter type remains the original sorted multimap;
conversion to the provider's container happens only at the send boundary. This
preserves endpoint/mock signatures and qualified wire behavior when the provider
changes its container/equality/query conventions.
Clients are configured once during SDK construction, not mutated while sending.
Internal helper consumers supplying their own httplib client must call
`internal::utils::http::configureClient` before using the request helper.
