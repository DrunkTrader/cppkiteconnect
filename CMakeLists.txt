# Modern CMake for cppkiteconnect v2
# Requires CMake 3.20+ for presets and modern features
cmake_minimum_required(VERSION 3.20)

# Project definition with version and languages
project(cppkiteconnect
    VERSION 2.0.0
    DESCRIPTION "Modern C++ SDK for Zerodha Kite Connect API"
    HOMEPAGE_URL "https://github.com/zerodha/cppkiteconnect"
    LANGUAGES CXX
)

# C++20 standard requirement
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set module path for custom find modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

# Build options
option(CPPKITECONNECT_BUILD_EXAMPLES "Build example programs" OFF)
option(CPPKITECONNECT_BUILD_TESTS "Build unit tests" OFF)
option(CPPKITECONNECT_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(CPPKITECONNECT_ENABLE_SANITIZERS "Enable sanitizers (ASan, UBSan, TSan)" OFF)
set(CPPKITECONNECT_SANITIZER_TYPE "address" CACHE STRING "Sanitizer type: address, undefined, thread")

# Installation directories (GNU standard)
include(GNUInstallDirs)

# Detect platform
if(UNIX AND NOT APPLE)
    message(STATUS "Building on Linux")
    set(CPPKITECONNECT_PLATFORM_LINUX TRUE)
elseif(APPLE)
    message(STATUS "Building on macOS")
    set(CPPKITECONNECT_PLATFORM_MACOS TRUE)
elseif(WIN32)
    message(STATUS "Building on Windows")
    set(CPPKITECONNECT_PLATFORM_WINDOWS TRUE)
endif()

# Sanitizer configuration
if(CPPKITECONNECT_ENABLE_SANITIZERS)
    if(CPPKITECONNECT_SANITIZER_TYPE STREQUAL "address")
        set(SANITIZER_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
        message(STATUS "Enabling AddressSanitizer")
    elseif(CPPKITECONNECT_SANITIZER_TYPE STREQUAL "undefined")
        set(SANITIZER_FLAGS "-fsanitize=undefined")
        message(STATUS "Enabling UndefinedBehaviorSanitizer")
    elseif(CPPKITECONNECT_SANITIZER_TYPE STREQUAL "thread")
        set(SANITIZER_FLAGS "-fsanitize=thread")
        message(STATUS "Enabling ThreadSanitizer")
    endif()
    
    if(SANITIZER_FLAGS)
        add_compile_options(${SANITIZER_FLAGS})
        add_link_options(${SANITIZER_FLAGS})
    endif()
endif()

# Find required dependencies using modern target-based approach
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# Find Boost components (asio, beast, url)
if(DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    # Using vcpkg - Boost will be provided by vcpkg
    find_package(Boost REQUIRED COMPONENTS asio beast url)
    message(STATUS "Using vcpkg-provided Boost ${Boost_VERSION_STRING}")
else()
    # Try system Boost
    find_package(Boost COMPONENTS asio beast url)
    if(Boost_FOUND)
        message(STATUS "Using system Boost ${Boost_VERSION_STRING}")
    else()
        message(WARNING "Boost not found. Some features may not work.")
    endif()
endif()

# Find nlohmann_json
find_package(nlohmann_json QUIET)
if(nlohmann_json_FOUND)
    message(STATUS "Found nlohmann_json ${nlohmann_json_VERSION}")
endif()

# Find fmt
find_package(fmt QUIET)
if(fmt_FOUND)
    message(STATUS "Found fmt ${fmt_VERSION}")
endif()

# Find GTest/GMock for tests
if(CPPKITECONNECT_BUILD_TESTS)
    find_package(GTest REQUIRED)
    find_package(GMock REQUIRED)
    message(STATUS "GoogleTest: ${GTEST_VERSION}")
    message(STATUS "GoogleMock: ${GMOCK_VERSION}")
endif()

# Find benchmark library
if(CPPKITECONNECT_BUILD_BENCHMARKS)
    find_package(benchmark QUIET)
    if(benchmark_FOUND)
        message(STATUS "Found Google Benchmark ${benchmark_VERSION}")
    endif()
endif()

# Collect source files for the library
set(CPPKITECONNECT_SOURCES
    src/types.cpp
    src/errors.cpp
    src/http_client.cpp
    src/mock_http_client.cpp
    src/json.cpp
    src/url_builder.cpp
    src/crypto.cpp
    src/csv_parser.cpp
    src/binary_decoder.cpp
    src/ticker.cpp
    src/ticker_public.cpp
    src/client.cpp
)

set(CPPKITECONNECT_HEADERS
    include/cppkiteconnect/types.hpp
    include/cppkiteconnect/errors.hpp
    include/cppkiteconnect/requests.hpp
    include/cppkiteconnect/client.hpp
    include/cppkiteconnect/json_utils.hpp
    include/cppkiteconnect/http_client.hpp
    include/cppkiteconnect/mock_http_client.hpp
    include/cppkiteconnect/url_builder.hpp
    include/cppkiteconnect/crypto.hpp
    include/cppkiteconnect/csv_parser.hpp
    include/cppkiteconnect/ticker.hpp
    include/cppkiteconnect/binary_decoder.hpp
    include/cppkiteconnect/internal/ticker_impl.hpp
    include/cppkiteconnect/responses/market.hpp
    include/cppkiteconnect/responses/user.hpp
    include/cppkiteconnect/responses/order.hpp
    include/cppkiteconnect/responses/portfolio.hpp
    include/cppkiteconnect/responses/gtt.hpp
    include/cppkiteconnect/responses/mf.hpp
    include/cppkiteconnect/responses/margins.hpp
    include/cppkiteconnect/responses/ws.hpp
)

# Create the main library target
add_library(cppkiteconnect ${CPPKITECONNECT_SOURCES} ${CPPKITECONNECT_HEADERS})
add_library(cppkiteconnect::cppkiteconnect ALIAS cppkiteconnect)

# Set target properties
set_target_properties(cppkiteconnect PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 2
    PUBLIC_HEADER "${CPPKITECONNECT_HEADERS}"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Include directories for the library and consumers
target_include_directories(cppkiteconnect
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(cppkiteconnect
    PUBLIC
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
        ZLIB::ZLIB
    PRIVATE
        Boost::asio
        Boost::beast
        Boost::url
        nlohmann_json::nlohmann_json
        fmt::fmt
)

# Enable coroutine support for compilers that need explicit flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(cppkiteconnect PRIVATE -fcoroutines)
endif()

# Note: Old BUILD_EXAMPLES, BUILD_TESTS, BUILD_DOCS options are deprecated
# Use CPPKITECONNECT_BUILD_EXAMPLES, CPPKITECONNECT_BUILD_TESTS, CPPKITECONNECT_BUILD_DOCS instead

# Build examples (deprecated: use CPPKITECONNECT_BUILD_EXAMPLES)
if(BUILD_EXAMPLES AND NOT CPPKITECONNECT_BUILD_EXAMPLES)
    set(CPPKITECONNECT_BUILD_EXAMPLES ON)
endif()

# Build tests (deprecated: use CPPKITECONNECT_BUILD_TESTS)
if(BUILD_TESTS AND NOT CPPKITECONNECT_BUILD_TESTS)
    set(CPPKITECONNECT_BUILD_TESTS ON)
endif()

# Build docs (deprecated: use CPPKITECONNECT_BUILD_DOCS)
if(BUILD_DOCS AND NOT CPPKITECONNECT_BUILD_DOCS)
    set(CPPKITECONNECT_BUILD_DOCS ON)
endif()

# Example targets placeholder - to be implemented in future phases
# Examples will use modern target-based approach with proper dependencies
if(CPPKITECONNECT_BUILD_EXAMPLES)
    message(STATUS "Example builds will be enabled after source migration (Phase 3+)")
    # TODO: Add example targets using modern CMake:
    # add_executable(example1 examples/example1.cpp)
    # target_link_libraries(example1 PRIVATE cppkiteconnect::cppkiteconnect)
endif()

# Test targets placeholder - to be implemented in future phases
# Tests will use GoogleTest with modern CMake integration
if(CPPKITECONNECT_BUILD_TESTS)
    message(STATUS "Building unit tests")
    enable_testing()
    
    # HTTP Client tests
    add_executable(test_http_client tests/unit/test_http_client.cpp)
    target_link_libraries(test_http_client PRIVATE 
        cppkiteconnect::cppkiteconnect 
        GTest::gtest_main
        GTest::gmock_main
    )
    set_target_properties(test_http_client PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(test_http_client PRIVATE -fcoroutines)
    endif()
    add_test(NAME http_client_tests COMMAND test_http_client)
    
    # Types tests (from Phase 3)
    if(EXISTS "${CMAKE_SOURCE_DIR}/tests/unit/test_types.cpp")
        add_executable(test_types tests/unit/test_types.cpp)
        target_link_libraries(test_types PRIVATE 
            cppkiteconnect::cppkiteconnect 
            GTest::gtest_main
        )
        set_target_properties(test_types PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
        )
        add_test(NAME types_tests COMMAND test_types)
    endif()

    # JSON parsing tests (Phase 5)
    if(EXISTS "${CMAKE_SOURCE_DIR}/tests/unit/test_json.cpp")
        add_executable(test_json tests/unit/test_json.cpp)
        target_link_libraries(test_json PRIVATE 
            cppkiteconnect::cppkiteconnect 
            GTest::gtest_main
        )
        set_target_properties(test_json PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
        )
        add_test(NAME json_tests COMMAND test_json)
    endif()
endif()

# Documentation target - to be enhanced in Phase 10
option(CPPKITECONNECT_BUILD_DOCS "Build documentation with Doxygen" OFF)
if(CPPKITECONNECT_BUILD_DOCS)
    find_package(Doxygen QUIET)
    
    if(Doxygen_FOUND)
        set(DOXYGEN_IN "${CMAKE_SOURCE_DIR}/cmake/templates/Doxyfile.in")
        set(DOXYGEN_OUT "${CMAKE_BINARY_DIR}/Doxyfile.out")
        
        if(EXISTS "${DOXYGEN_IN}")
            configure_file("${DOXYGEN_IN}" "${DOXYGEN_OUT}" @ONLY)
            
            add_custom_target(docs
                COMMAND ${Doxygen_EXECUTABLE} "${DOXYGEN_OUT}"
                WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM
            )
        else()
            message(WARNING "Doxyfile.in not found at ${DOXYGEN_IN}")
        endif()
    else()
        message(WARNING "Doxygen not found. Install doxygen to build documentation.")
    endif()
endif()

# Installation rules (placeholder for Phase 10)
# Will install headers, library, and CMake config files
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT Development
    FILES_MATCHING PATTERN "*.hpp"
)

# Export targets for downstream projects
# This enables find_package(cppkiteconnect) usage
export(EXPORT cppkiteconnectTargets
    FILE "${CMAKE_BINARY_DIR}/cppkiteconnectTargets.cmake"
    NAMESPACE cppkiteconnect::
)