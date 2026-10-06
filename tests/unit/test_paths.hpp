/*
 *  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 *  SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

namespace kiteconnect::test {

inline std::string testDataPath(const std::string& path) {
#ifdef KITE_TEST_DATA_DIR
    constexpr const char* legacyPrefix = "../tests";
    if (path.rfind(legacyPrefix, 0) == 0) {
        return std::string(KITE_TEST_DATA_DIR) + path.substr(8);
    }
#endif
    return path;
}

} // namespace kiteconnect::test
