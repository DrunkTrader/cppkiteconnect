/* SPDX-License-Identifier: MIT */
#include <cstdlib>
#include <iostream>
#include <kitepp.hpp>

namespace {

int documentedRestExample() {
    const char* key = std::getenv("KITE_API_KEY");
    const char* token = std::getenv("KITE_ACCESS_TOKEN");
    if (!key || !token) { return 2; }
    kiteconnect::kite client(key);
    client.setAccessToken(token);
    const auto profile = client.profile();
    std::cout << profile.userName << " <" << profile.email << ">\n";
    return 0;
}

int documentedTickerExample() {
    const char* key = std::getenv("KITE_API_KEY");
    const char* token = std::getenv("KITE_ACCESS_TOKEN");
    if (!key || !token) { return 2; }

    kiteconnect::ticker client(key);
    client.setAccessToken(token);
    client.onConnect = [](kiteconnect::ticker* ticker) {
        ticker->subscribe({ 408065 });
        ticker->setMode(kiteconnect::MODE_FULL, { 408065 });
    };
    client.onTicks = [](kiteconnect::ticker*,
        const std::vector<kiteconnect::tick>& ticks) {
        for (const auto& tick : ticks) {
            std::cout << tick.instrumentToken << ": " << tick.lastPrice << '\n';
        }
    };
    return 0;
}

} // namespace

int main() { return 0; }
