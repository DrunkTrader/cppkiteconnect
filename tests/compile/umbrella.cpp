#include <kitepp.hpp>

int main() {
    kiteconnect::kite client("compile-only");
    return client.getApiKey().empty() ? 1 : 0;
}
