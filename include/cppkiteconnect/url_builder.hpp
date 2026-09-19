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
#include <map>
#include <optional>

namespace cppkiteconnect {

/**
 * @brief URL builder using Boost.URL for constructing API endpoints
 * 
 * Provides a fluent interface for building URLs with query parameters
 */
class url_builder {
public:
    /**
     * @brief Construct a new url builder
     * @param base_url Base URL (e.g., "https://api.kite.trade")
     */
    explicit url_builder(std::string_view base_url);

    /**
     * @brief Set the path component
     * @param path Path to append (e.g., "/orders/regular")
     * @return Reference to this builder
     */
    url_builder& set_path(std::string_view path);

    /**
     * @brief Append to the path
     * @param segment Path segment to append
     * @return Reference to this builder
     */
    url_builder& append_path(std::string_view segment);

    /**
     * @brief Add a query parameter
     * @param key Parameter name
     * @param value Parameter value
     * @return Reference to this builder
     */
    url_builder& add_query_param(std::string_view key, std::string_view value);

    /**
     * @brief Add an optional query parameter (only if has value)
     * @param key Parameter name
     * @param value Optional parameter value
     * @return Reference to this builder
     */
    template<typename T>
    url_builder& add_optional_param(std::string_view key, std::optional<T> const& value) {
        if (value.has_value()) {
            if constexpr (std::is_same_v<T, std::string>) {
                add_query_param(key, *value);
            } else {
                add_query_param(key, std::to_string(*value));
            }
        }
        return *this;
    }

    /**
     * @brief Build the final URL string
     * @return Complete URL with scheme, host, path, and query string
     */
    [[nodiscard]] std::string build() const;

    /**
     * @brief Get the base URL
     * @return Base URL
     */
    [[nodiscard]] std::string base_url() const { return base_url_; }

    /**
     * @brief Get the current path
     * @return Current path
     */
    [[nodiscard]] std::string path() const { return path_; }

    /**
     * @brief Clear all query parameters
     * @return Reference to this builder
     */
    url_builder& clear_params();

private:
    std::string base_url_;
    std::string path_;
    std::map<std::string, std::string> params_;
};

} // namespace cppkiteconnect
