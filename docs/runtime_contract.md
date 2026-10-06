# Runtime contract

## REST and model parsing

REST remains synchronous. Replacing the API key or access token updates the
Authorization header used by subsequent calls. SDK-generated diagnostics omit
request URLs and response bodies. API error messages remain service-supplied.

JSON must be valid UTF-8 without raw embedded NUL bytes. Root, envelope, container,
element, and scalar types are checked before accessing them. Missing scalar
members retain compatibility defaults (empty string, false, zero); nullable
strings become empty strings. Present members of the wrong type throw
`libException`. Integer fields require the declared representable integer range;
double fields accept JSON numbers and use ordinary binary64 conversion, which
does not promise exact representation of integers above 2^53. HTTP 200 error
envelopes are errors. A 200 response with `data` but without `status` remains
accepted for compatibility with the pinned mutual-fund fixture.

CSV numbers must consume the whole decimal field, fit the target type, and be
finite; empty numeric fields remain zero. Parsing is locale-independent, including
on standard libraries without floating-point `from_chars`. Leading plus/whitespace,
hexadecimal syntax, trailing data, overflow and underflow to zero are rejected;
representable subnormal values are accepted. Short CSV rows and candles are rejected.
Reparsing collection DTOs replaces their vectors and does not consume the source
DOM. The legacy internal container-extraction overload taking `rj::Value& out`
still transfers ownership; DTO parsers no longer use it. Public RapidJSON parsing
signatures and scalar defaults remain compatibility debt for a later major API.

Required request scalars start in deterministic invalid/default states. Invalid
orders, SIPs, and GTT modifications fail before transport. SIP installments `-1`
remain supported for an indefinite schedule.

## Ticker ownership and callbacks

Configure the ticker, call `connect()`, and call `run()` on the selected owner
thread. While `run()` is active, credentials, subscriptions, modes and callback
fields must be accessed/mutated only on that thread. Calls from callbacks are
allowed. `stop()`, `isConnected()` and `getLastBeatTime()` support cross-thread use;
other operations are not a general thread-safe interface.

Callbacks run on the `run()` thread. Reference arguments are borrowed only for
that invocation; copy them to retain data. Callback exceptions cause a generic
error callback and terminal stop. An exception from `onError` stops the ticker
without recursively calling `onError`. Destroy the ticker only after `run()` has
returned and its thread has been joined; never delete it inside a callback.
Ticker copy and move are disabled.

C++20 binary decoding uses checked `std::span<const char>` views into the received
frame and fixed-size stack fields. These views never escape decoding: returned
ticks/depth and copied callback values still own their data. The C++17 rollback
decoder retains its protected vector-based implementation.

`stop()` is idempotent and terminal. It cancels pending connect and retry work,
drops queued commands while preserving any in-flight buffer, initiates close on
an open connection, and forcibly cancels after `tickerOptions::closeTimeout`.
Handlers drain before `run()` returns. An open session reports `onClose` once;
an unsuccessful pre-open connection uses `onConnectError`. Reusing a stopped
ticker via `connect()` is rejected; construct a new instance.

Subscribe before calling `setMode`. Commands preserve order. Reconnect replays
explicit subscribe before mode commands, then invokes `onConnect`. Desired
subscriptions change only after successful queue admission; admission is not a
server acknowledgment. Queue and incoming-message bounds are configurable in
`tickerOptions`. Invalid modes/tokens, more than 3000 subscriptions, and queue
overflow throw `libException`.

## Trust, deadlines and reconnect

WSS always verifies the peer certificate chain and hostname, sets SNI, and uses
OpenSSL's default trust paths or `tickerOptions::caFile`. There is no insecure
TLS mode. REST uses cpp-httplib's default peer/hostname verification. A deployment
must supply a usable system trust store.

Connect and writes have bounded deadlines. WebSocket maintenance uses Beast's
idle/ping timer; retry uses a cancellable steady timer and capped exponential
delay. TLS failures, HTTP upgrade 401/403, policy close 1008, and normal close
1000 do not retry. Transient failures retry only when enabled and within the
configured attempt budget. Remote close codes/reasons are delivered to
`onClose`; abrupt failures use 1006. No REST order is automatically retried.

The public heartbeat timestamp remains a `system_clock` value for compatibility;
it records the most recent application one-byte heartbeat, even without a ticks
callback. It is not a monotonic elapsed-time deadline.
