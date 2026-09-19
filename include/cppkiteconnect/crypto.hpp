/*
 *  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 *  SPDX-License-Identifier: MIT
 *
 *  Copyright (c) 2020-2024 Bhumit Attarde.
 *
 *  Permission is hereby  granted, free of charge, to any  person obtaining a
 * copy of this software and associated  documentation files (the "Software"),
 * to deal in the Software  without restriction, including without  limitation
 * the rights to  use, copy,  modify, merge,  publish, distribute,  sublicense,
 * and/or  sell copies  of  the Software,  and  to  permit persons  to  whom the
 * Software  is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS
 * OR IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN
 * NO EVENT  SHALL THE AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY
 * CLAIM,  DAMAGES OR  OTHER LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <string>
#include <string_view>
#include <span>
#include <vector>
#include <cstdint>

namespace cppkiteconnect {

/**
 * @brief SHA-256 hash utility using OpenSSL EVP
 * 
 * Provides secure hashing for session generation and other cryptographic needs
 */
class sha256_hasher {
public:
    /**
     * @brief Compute SHA-256 hash of data
     * @param data Input data
     * @return Hex-encoded hash string (64 characters)
     */
    [[nodiscard]] static std::string hash(std::string_view data);

    /**
     * @brief Compute SHA-256 hash of binary data
     * @param data Pointer to binary data
     * @param size Size of data in bytes
     * @return Hex-encoded hash string (64 characters)
     */
    [[nodiscard]] static std::string hash(void const* data, std::size_t size);

    /**
     * @brief Compute SHA-256 hash and return raw bytes
     * @param data Input data
     * @return Raw hash bytes (32 bytes)
     */
    [[nodiscard]] static std::vector<std::uint8_t> hash_bytes(std::string_view data);

    /**
     * @brief Compute SHA-256 hash and return raw bytes
     * @param data Pointer to binary data
     * @param size Size of data in bytes
     * @return Raw hash bytes (32 bytes)
     */
    [[nodiscard]] static std::vector<std::uint8_t> hash_bytes(void const* data, std::size_t size);

    /**
     * @brief Hash size in bytes
     */
    static constexpr std::size_t hash_size = 32;

    /**
     * @brief Hex-encoded hash size
     */
    static constexpr std::size_t hex_hash_size = 64;
};

} // namespace cppkiteconnect
