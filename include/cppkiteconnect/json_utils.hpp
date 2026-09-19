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

namespace cppkiteconnect::json {

/**
 * @brief Serialize value to JSON string
 * @param value Value to serialize
 * @return JSON string representation
 */
template<typename T>
[[nodiscard]] std::string serialize(T const& value);

/**
 * @brief Deserialize JSON string to value
 * @param json_str JSON string
 * @return Deserialized value
 */
template<typename T>
[[nodiscard]] T deserialize(std::string_view json_str);

/**
 * @brief Parse JSON object field with error handling
 * @param json_obj JSON object
 * @param field Field name
 * @return Field value or throws parse_error
 */
template<typename T>
[[nodiscard]] T get_field(void const* json_obj, std::string_view field);

/**
 * @brief Parse optional JSON object field
 * @param json_obj JSON object
 * @param field Field name
 * @return Optional field value (empty if not present)
 */
template<typename T>
[[nodiscard]] std::optional<T> get_optional_field(void const* json_obj, std::string_view field);

} // namespace cppkiteconnect::json
