#include <kitepp/rest.hpp>
#include <kitepp/ticker.hpp>

int tickerConsumerA() {
    kiteconnect::ticker client("compile-only");
    return client.isConnected() ? 1 : 0;
}
