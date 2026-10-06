# Generated translation units are validation sources in the build tree, not an
# SDK implementation/library. Each TU includes exactly one first-party header.
file(GLOB_RECURSE headers CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/kitepp/*.hpp")
list(APPEND headers kitepp.hpp)
set(check_sources "${CMAKE_CURRENT_SOURCE_DIR}/tests/compile/header_main.cpp")
foreach(header IN LISTS headers)
    if(NOT KITEPP_ENABLE_TICKER AND header MATCHES "^(kitepp\\.hpp|kitepp/ticker)")
        continue()
    endif()
    string(MAKE_C_IDENTIFIER "${header}" identifier)
    set(source "${CMAKE_CURRENT_BINARY_DIR}/header-checks/${identifier}.cpp")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/header_check.cpp.in"
        "${source}" @ONLY)
    list(APPEND check_sources "${source}")
endforeach()
add_executable(kitepp-header-checks ${check_sources})
if(KITEPP_ENABLE_TICKER)
    target_link_libraries(kitepp-header-checks PRIVATE kitepp::kitepp)
else()
    target_link_libraries(kitepp-header-checks PRIVATE kitepp::rest)
endif()
kitepp_strict_standard(kitepp-header-checks)
