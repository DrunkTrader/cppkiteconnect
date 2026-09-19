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
#include <vector>
#include <optional>
#include <functional>

namespace cppkiteconnect {

/**
 * @brief Represents a single instrument/trading symbol
 */
struct instrument {
    std::string exchange;
    std::string trading_symbol;
    std::string instrument_token;
    std::string instrument_type;
    std::string product;
    int lot_size = 0;
    unsigned int strike = 0;
    double tick_size = 0.0;
    double multiplier = 0.0;
    std::string expiry;
};

/**
 * @brief CSV parser for instrument data and other tabular data
 * 
 * Minimal implementation for parsing Kite Connect instrument CSV files
 */
class csv_parser {
public:
    /**
     * @brief Parse CSV data into rows
     * @param csv_data CSV content as string
     * @param has_header Whether first row is header
     * @return Vector of rows, each row is vector of fields
     */
    [[nodiscard]] static std::vector<std::vector<std::string>> parse(
        std::string_view csv_data, 
        bool has_header = true);

    /**
     * @brief Parse instrument CSV specifically
     * @param csv_data CSV content from Kite Connect instruments endpoint
     * @return Vector of instrument structures
     */
    [[nodiscard]] static std::vector<instrument> parse_instruments(std::string_view csv_data);

    /**
     * @brief Parse CSV from file
     * @param filepath Path to CSV file
     * @param has_header Whether first row is header
     * @return Vector of rows
     * @throws std::runtime_error if file cannot be read
     */
    [[nodiscard]] static std::vector<std::vector<std::string>> parse_file(
        std::string_view filepath, 
        bool has_header = true);

    /**
     * @brief Parse instrument CSV from file
     * @param filepath Path to instrument CSV file
     * @return Vector of instruments
     * @throws std::runtime_error if file cannot be read
     */
    [[nodiscard]] static std::vector<instrument> parse_instruments_file(std::string_view filepath);
};

/**
 * @brief Field converter utilities for CSV parsing
 */
class field_converter {
public:
    /**
     * @brief Convert string field to integer
     * @param field String field value
     * @param default_value Default if conversion fails
     * @return Converted integer
     */
    [[nodiscard]] static int to_int(std::string_view field, int default_value = 0);

    /**
     * @brief Convert string field to unsigned int
     * @param field String field value
     * @param default_value Default if conversion fails
     * @return Converted unsigned int
     */
    [[nodiscard]] static unsigned int to_uint(std::string_view field, unsigned int default_value = 0);

    /**
     * @brief Convert string field to double
     * @param field String field value
     * @param default_value Default if conversion fails
     * @return Converted double
     */
    [[nodiscard]] static double to_double(std::string_view field, double default_value = 0.0);

    /**
     * @brief Convert string field to optional string
     * @param field String field value
     * @return Optional string (empty if field is empty)
     */
    [[nodiscard]] static std::optional<std::string> to_optional_string(std::string_view field);
};

} // namespace cppkiteconnect
