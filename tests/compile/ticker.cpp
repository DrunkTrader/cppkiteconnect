#include <kitepp/ticker.hpp>

int main() {
    kiteconnect::ticker client("compile-only");
    return client.isConnected() ? 1 : 0;
}
