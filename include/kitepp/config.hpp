/* SPDX-License-Identifier: MIT */
#pragma once

static_assert(sizeof(void*) == 8, "kitepp requires a 64-bit platform");

#ifdef _MSVC_LANG
#define KITEPP_CPLUSPLUS _MSVC_LANG
#else
#define KITEPP_CPLUSPLUS __cplusplus
#endif
#ifdef KITEPP_CPP17_COMPAT
static_assert(KITEPP_CPLUSPLUS >= 201703L, "kitepp compatibility mode requires C++17");
#else
static_assert(KITEPP_CPLUSPLUS >= 202002L, "kitepp requires C++20; use the explicit C++17 compatibility configuration only for rollback qualification");
#endif

// All SDK entry points use one header-only/TLS configuration. CMake propagates
// these definitions before any include; manual users receive the same defaults.
#if defined(CPPHTTPLIB_HTTPLIB_H) && !defined(CPPHTTPLIB_OPENSSL_SUPPORT)
#error "Include kitepp/config.hpp before cpp-httplib, or define CPPHTTPLIB_OPENSSL_SUPPORT consistently in every translation unit"
#endif
#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif

#if defined(FMT_VERSION) && !defined(FMT_HEADER_ONLY)
#error "Include kitepp/config.hpp before fmt, or define FMT_HEADER_ONLY consistently in every translation unit"
#endif
#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY 1
#endif
