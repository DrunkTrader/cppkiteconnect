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

#include "cppkiteconnect/url_builder.hpp"

#include <sstream>
#include <algorithm>

namespace cppkiteconnect {

url_builder::url_builder(std::string_view base_url) 
    : base_url_(base_url) {
    // Remove trailing slash from base URL if present
    while (!base_url_.empty() && base_url_.back() == '/') {
        base_url_.pop_back();
    }
}

url_builder& url_builder::set_path(std::string_view path) {
    path_ = path;
    // Ensure path starts with /
    if (!path_.empty() && path_.front() != '/') {
        path_ = "/" + path_;
    }
    return *this;
}

url_builder& url_builder::append_path(std::string_view segment) {
    if (path_.empty()) {
        path_ = "/";
    }
    
    // Ensure path doesn't end with double slash
    if (!path_.empty() && path_.back() == '/') {
        path_.pop_back();
    }
    
    // Add segment
    if (!segment.empty()) {
        if (segment.front() == '/') {
            path_ += segment;
        } else {
            path_ += "/" + std::string(segment);
        }
    }
    
    return *this;
}

url_builder& url_builder::add_query_param(std::string_view key, std::string_view value) {
    params_[std::string(key)] = std::string(value);
    return *this;
}

std::string url_builder::build() const {
    std::ostringstream oss;
    
    // Add base URL
    oss << base_url_;
    
    // Add path
    if (!path_.empty()) {
        oss << path_;
    }
    
    // Add query string
    if (!params_.empty()) {
        oss << "?";
        bool first = true;
        for (auto const& [key, value] : params_) {
            if (!first) {
                oss << "&";
            }
            oss << key << "=" << value;
            first = false;
        }
    }
    
    return oss.str();
}

url_builder& url_builder::clear_params() {
    params_.clear();
    return *this;
}

} // namespace cppkiteconnect
