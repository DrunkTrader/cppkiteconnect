/*
 *  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 *  SPDX-License-Identifier: MIT
 *
 *  Copyright (c) 2020-2022 Bhumit Attarde
 *  Copyright (c) 2024 C++ Kite Connect Migration
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
#include <vector>
#include <optional>

#include <nlohmann/json.hpp>

namespace cppkiteconnect {

using json = nlohmann::json;

/// Represents an user's profile.
struct user_profile {
    std::string user_id;
    std::string user_name;
    std::string user_shortname;
    std::string avatar_url;
    std::string user_type;
    std::string email;
    std::string broker;
    std::vector<std::string> products;
    std::vector<std::string> order_types;
    std::vector<std::string> exchanges;
    
    struct meta {
        std::string demat_consent;
    };
    meta meta_info;
};

/// Represents tokens received after successful authentication.
struct user_tokens {
    std::string user_id;
    std::string access_token;
    std::string refresh_token;
};

/// Represents information of a user session.
struct user_session {
    std::string api_key;
    std::string public_token;
    std::string login_time;
    user_profile profile;
    user_tokens tokens;
};

/// Represents available margins of a single segment.
struct available_margins {
    double adhoc_margin = -1.0;
    double cash = -1.0;
    double collateral = -1.0;
    double intraday_payin = -1.0;
};

/// Represents used margins of a single segment.
struct used_margins {
    double debits = -1.0;
    double exposure = -1.0;
    double m2m_realised = -1.0;
    double m2m_unrealised = -1.0;
    double option_premium = -1.0;
    double payout = -1.0;
    double span = -1.0;
    double holding_sales = -1.0;
    double turnover = -1.0;
};

/// Represents user margins for a segment.
struct margins {
    double net = -1.0;
    bool enabled = false;
    available_margins available;
    used_margins utilised;
};

/// Represents all margins (equity and commodity).
struct all_margins {
    margins equity;
    margins commodity;
};

// JSON serialization helpers
void from_json(const json& j, user_profile::meta& m);
void from_json(const json& j, user_profile& p);
void from_json(const json& j, user_tokens& t);
void from_json(const json& j, user_session& s);
void from_json(const json& j, available_margins& m);
void from_json(const json& j, used_margins& m);
void from_json(const json& j, margins& m);
void from_json(const json& j, all_margins& m);

// Parser functions for client API
[[nodiscard]] user_session parse_auth_response(json const& j);
[[nodiscard]] user_profile parse_user_profile(json const& j);
[[nodiscard]] margins parse_margins_response(json const& j);

} // namespace cppkiteconnect
