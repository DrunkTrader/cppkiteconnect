/* SPDX-License-Identifier: MIT */
#include <kitepp/ticker.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>

namespace {
std::atomic<size_t> allocations { 0 };
std::atomic<bool> countAllocations { false };
}

void* operator new(std::size_t size) {
    if (void* memory = std::malloc(size ? size : 1)) {
        if (countAllocations.load(std::memory_order_relaxed)) {
            allocations.fetch_add(1, std::memory_order_relaxed);
        }
        return memory;
    }
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

namespace kiteconnect {
// Access is a permanent private validation seam, not a macro-dependent layout.
class tickerDecoderBenchmark {
  public:
    static int run() {
        ticker client("offline-benchmark");
        constexpr size_t iterations = 5000;
        for (size_t packets : { 1U, 16U, 64U }) {
            std::vector<char> frame(2 + packets * 186, '\0');
            frame[1] = static_cast<char>(packets);
            for (size_t packet = 0; packet < packets; ++packet) {
                const size_t start = 2 + packet * 186;
                frame[start + 1] = static_cast<char>(184);
                frame[start + 5] = 1;   // tradable instrument token
                frame[start + 9] = 100; // price = 1.00
                frame[start + 45] = 100; // close = 1.00
            }
            for (size_t index = 0; index < 100; ++index) {
                client.parseBinaryMessage(frame.data(), frame.size());
            }
            std::vector<int64_t> samples(iterations);
            double checksum = 0;
            allocations = 0;
            countAllocations = true;
            const auto begin = std::chrono::steady_clock::now();
            for (size_t index = 0; index < iterations; ++index) {
                const auto start = std::chrono::steady_clock::now();
                const auto ticks = client.parseBinaryMessage(frame.data(), frame.size());
                for (const auto& tick : ticks) { checksum += tick.lastPrice; }
                samples[index] = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - start).count();
            }
            const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - begin).count();
            countAllocations = false;
            std::sort(samples.begin(), samples.end());
            if (checksum != iterations * packets) { return 1; }
            std::cout << "cpp=" << KITEPP_CPLUSPLUS << " packets=" << packets
                      << " allocations/frame=" << double(allocations) / iterations
                      << " mean_ns=" << elapsed / iterations
                      << " p50_ns=" << samples[iterations / 2]
                      << " p99_ns=" << samples[iterations * 99 / 100]
                      << " checksum=" << checksum << '\n';
        }
        return 0;
    }
};
}

int main() { return kiteconnect::tickerDecoderBenchmark::run(); }
