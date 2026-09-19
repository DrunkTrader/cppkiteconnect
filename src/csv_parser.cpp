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

#include "cppkiteconnect/csv_parser.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <charconv>

namespace cppkiteconnect {

namespace detail {

std::vector<std::string> split_line(std::string_view line, char delimiter = ',') {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;
    
    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        
        if (c == '"') {
            // Check for escaped quote
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
                field += '"';
                ++i; // Skip next quote
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == delimiter && !in_quotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    
    fields.push_back(field);
    return fields;
}

std::string trim(std::string_view str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(static_cast<unsigned char>(*start))) {
        ++start;
    }
    
    auto end = str.end();
    do {
        --end;
    } while (std::distance(start, end) > 0 && std::isspace(static_cast<unsigned char>(*end)));
    
    return std::string(start, end + 1);
}

} // namespace detail

std::vector<std::vector<std::string>> csv_parser::parse(
    std::string_view csv_data, 
    bool has_header) {
    
    std::vector<std::vector<std::string>> rows;
    std::istringstream iss(std::string(csv_data));
    std::string line;
    
    bool first_line = true;
    
    while (std::getline(iss, line)) {
        // Skip empty lines
        if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }
        
        // Remove carriage return if present (Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // Skip header if requested
        if (has_header && first_line) {
            first_line = false;
            continue;
        }
        
        first_line = false;
        rows.push_back(detail::split_line(line));
    }
    
    return rows;
}

std::vector<instrument> csv_parser::parse_instruments(std::string_view csv_data) {
    auto rows = parse(csv_data, true); // Has header
    std::vector<instrument> instruments;
    instruments.reserve(rows.size());
    
    for (auto const& row : rows) {
        if (row.size() < 9) {
            continue; // Skip malformed rows
        }
        
        instrument inst;
        inst.exchange = detail::trim(row[0]);
        inst.trading_symbol = detail::trim(row[1]);
        inst.instrument_token = detail::trim(row[2]);
        inst.instrument_type = detail::trim(row[3]);
        inst.product = detail::trim(row[5]); // Column 4 is name
        inst.lot_size = field_converter::to_int(detail::trim(row[6]));
        inst.strike = field_converter::to_uint(detail::trim(row[7]));
        inst.tick_size = field_converter::to_double(detail::trim(row[8]));
        
        // Optional columns
        if (row.size() > 9) {
            inst.multiplier = field_converter::to_double(detail::trim(row[9]));
        }
        if (row.size() > 10) {
            inst.expiry = detail::trim(row[10]);
        }
        
        instruments.push_back(inst);
    }
    
    return instruments;
}

std::vector<std::vector<std::string>> csv_parser::parse_file(
    std::string_view filepath, 
    bool has_header) {
    
    std::ifstream file(std::string(filepath));
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + std::string(filepath));
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return parse(buffer.str(), has_header);
}

std::vector<instrument> csv_parser::parse_instruments_file(std::string_view filepath) {
    std::ifstream file(std::string(filepath));
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + std::string(filepath));
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return parse_instruments(buffer.str());
}

int field_converter::to_int(std::string_view field, int default_value) {
    if (field.empty()) {
        return default_value;
    }
    
    int result = 0;
    auto [ptr, ec] = std::from_chars(field.data(), field.data() + field.size(), result);
    
    if (ec == std::errc{}) {
        return result;
    }
    return default_value;
}

unsigned int field_converter::to_uint(std::string_view field, unsigned int default_value) {
    if (field.empty()) {
        return default_value;
    }
    
    unsigned int result = 0;
    auto [ptr, ec] = std::from_chars(field.data(), field.data() + field.size(), result);
    
    if (ec == std::errc{}) {
        return result;
    }
    return default_value;
}

double field_converter::to_double(std::string_view field, double default_value) {
    if (field.empty()) {
        return default_value;
    }
    
    try {
        std::size_t pos = 0;
        double result = std::stod(std::string(field), &pos);
        if (pos == field.size()) {
            return result;
        }
    } catch (...) {
        // Fall through to default
    }
    
    return default_value;
}

std::optional<std::string> field_converter::to_optional_string(std::string_view field) {
    if (field.empty()) {
        return std::nullopt;
    }
    
    std::string trimmed = detail::trim(field);
    if (trimmed.empty()) {
        return std::nullopt;
    }
    
    return trimmed;
}

} // namespace cppkiteconnect
