#include <kitepp/rest.hpp>
#include <kitepp/responses/market.hpp>

const char* compileConsumerB() {
    kiteconnect::kite client("compile-only");
    return client.getApiKey().empty() ? "" : kiteconnect::MODE_FULL.c_str();
}
