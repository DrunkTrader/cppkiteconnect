#include <kitepp/responses/market.hpp>
#include <kitepp/rest.hpp>

const char* compileConsumerA() {
    kiteconnect::kite client("compile-only");
    return client.getApiKey().empty() ? "" : kiteconnect::MODE_LTP.c_str();
}
