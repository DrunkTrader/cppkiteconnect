#include <kitepp/ticker.hpp>
#include <kitepp/rest.hpp>

int tickerConsumerB() {
    kiteconnect::ticker client("compile-only");
    return client.isConnected() ? 1 : 0;
}
