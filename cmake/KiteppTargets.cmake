find_package(Threads REQUIRED)
find_package(OpenSSL 3.0 REQUIRED)
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The modernized SDK requires a 64-bit platform")
endif()

# Models still include utils.hpp and therefore HTTP/CSV/fmt. This is their real
# closure, not a claim that public RapidJSON DTOs are transport independent.
add_library(kitepp_models INTERFACE)
add_library(kitepp::models ALIAS kitepp_models)
set_target_properties(kitepp_models PROPERTIES EXPORT_NAME models)
target_compile_features(kitepp_models INTERFACE cxx_std_${KITEPP_CXX_STANDARD})
if(KITEPP_CXX_STANDARD STREQUAL "17")
    target_compile_definitions(kitepp_models INTERFACE KITEPP_CPP17_COMPAT=1)
endif()
target_include_directories(kitepp_models INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_compile_definitions(kitepp_models INTERFACE
    CPPHTTPLIB_OPENSSL_SUPPORT FMT_HEADER_ONLY=1)
target_link_libraries(kitepp_models INTERFACE OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
if(WIN32)
    target_link_libraries(kitepp_models INTERFACE ws2_32 crypt32)
endif()

add_library(kitepp_rest INTERFACE)
add_library(kitepp::rest ALIAS kitepp_rest)
set_target_properties(kitepp_rest PROPERTIES EXPORT_NAME rest)
target_link_libraries(kitepp_rest INTERFACE kitepp_models)

if(KITEPP_ENABLE_TICKER)
    find_package(Boost 1.83 CONFIG REQUIRED)
    add_library(kitepp_ticker INTERFACE)
    add_library(kitepp::ticker ALIAS kitepp_ticker)
    set_target_properties(kitepp_ticker PROPERTIES EXPORT_NAME ticker)
    target_link_libraries(kitepp_ticker INTERFACE kitepp_models Boost::headers)
    add_library(kitepp INTERFACE)
    add_library(kitepp::kitepp ALIAS kitepp)
    set_target_properties(kitepp PROPERTIES EXPORT_NAME kitepp)
    target_link_libraries(kitepp INTERFACE kitepp_rest kitepp_ticker)
endif()

function(kitepp_strict_standard target)
    set_target_properties(${target} PROPERTIES CXX_STANDARD ${KITEPP_CXX_STANDARD}
        CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
endfunction()
