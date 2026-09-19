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

#include "cppkiteconnect/crypto.hpp"

#include <openssl/evp.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace cppkiteconnect {

std::string sha256_hasher::hash(std::string_view data) {
    auto bytes = hash_bytes(data);
    
    // Convert to hex string
    std::ostringstream oss;
    for (auto byte : bytes) {
        oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

std::string sha256_hasher::hash(void const* data, std::size_t size) {
    auto bytes = hash_bytes(data, size);
    
    // Convert to hex string
    std::ostringstream oss;
    for (auto byte : bytes) {
        oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

std::vector<std::uint8_t> sha256_hasher::hash_bytes(std::string_view data) {
    return hash_bytes(data.data(), data.size());
}

std::vector<std::uint8_t> sha256_hasher::hash_bytes(void const* data, std::size_t size) {
    std::vector<std::uint8_t> result(hash_size);
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }
    
    try {
        if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
            throw std::runtime_error("Failed to initialize SHA-256 digest");
        }
        
        if (EVP_DigestUpdate(ctx, data, size) != 1) {
            throw std::runtime_error("Failed to update digest");
        }
        
        unsigned int len = 0;
        if (EVP_DigestFinal_ex(ctx, result.data(), &len) != 1) {
            throw std::runtime_error("Failed to finalize digest");
        }
        
        if (len != hash_size) {
            throw std::runtime_error("Unexpected hash length");
        }
    } catch (...) {
        EVP_MD_CTX_free(ctx);
        throw;
    }
    
    EVP_MD_CTX_free(ctx);
    return result;
}

} // namespace cppkiteconnect
