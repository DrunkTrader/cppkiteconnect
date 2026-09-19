# FindBoostComponents.cmake
# Modern CMake module to find required Boost components
# This is a helper module for cppkiteconnect project
#
# Finds the following Boost components:
#   - asio
#   - beast
#   - url
#
# Defines the following imported targets:
#   Boost::asio
#   Boost::beast
#   Boost::url

include(FindPackageHandleStandardArgs)

# Check if using vcpkg or system Boost
if(DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    # vcpkg will handle finding Boost via config mode
    find_package(Boost REQUIRED COMPONENTS asio beast url)
else()
    # Try to find Boost with required components
    find_package(Boost REQUIRED COMPONENTS asio beast url)
endif()

# Verify all components were found
set(_required_components asio beast url)
foreach(comp ${_required_components})
    if(NOT TARGET Boost::${comp})
        message(FATAL_ERROR "Boost component '${comp}' not found. Please ensure vcpkg or system Boost has this component.")
    endif()
endforeach()

message(STATUS "Found Boost ${Boost_VERSION_STRING}")
message(STATUS "  Asio: Boost::asio")
message(STATUS "  Beast: Boost::beast (header-only)")
message(STATUS "  URL: Boost::url")
