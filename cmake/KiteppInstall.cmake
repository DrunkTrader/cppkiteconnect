include(CMakePackageConfigHelpers)
if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}" OR IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
    message(FATAL_ERROR "Relocatable kitepp installs require relative include/lib directories")
endif()
set(kitepp_config_dir "${CMAKE_INSTALL_LIBDIR}/cmake/kitepp")
install(TARGETS kitepp_models kitepp_rest EXPORT kiteppCoreTargets)
install(EXPORT kiteppCoreTargets NAMESPACE kitepp:: DESTINATION "${kitepp_config_dir}")
if(KITEPP_ENABLE_TICKER)
    install(TARGETS kitepp_ticker kitepp EXPORT kiteppTickerTargets)
    install(EXPORT kiteppTickerTargets NAMESPACE kitepp:: DESTINATION "${kitepp_config_dir}")
endif()
configure_package_config_file(cmake/templates/kiteppConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/kiteppConfig.cmake"
    INSTALL_DESTINATION "${kitepp_config_dir}")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/kiteppConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMajorVersion ARCH_INDEPENDENT)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/kiteppConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/kiteppConfigVersion.cmake" DESTINATION "${kitepp_config_dir}")
install(DIRECTORY include/kitepp DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(FILES include/kitepp.hpp DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
# Bundle only the required pinned headers, not third-party sources/build trees.
install(FILES include/cpp-httplib/httplib.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cpp-httplib")
install(DIRECTORY include/fmt/include/fmt DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/fmt/include")
install(DIRECTORY include/rapidjson/include/rapidjson DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/rapidjson/include")
install(FILES include/rapidcsv/src/rapidcsv.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/rapidcsv/src")
install(FILES include/PicoSHA2/picosha2.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/PicoSHA2")
install(FILES LICENSE DESTINATION "${CMAKE_INSTALL_DATADIR}/kitepp")
foreach(dependency cpp-httplib fmt rapidjson rapidcsv PicoSHA2)
    file(GLOB license "${CMAKE_CURRENT_SOURCE_DIR}/include/${dependency}/LICENSE*"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/${dependency}/license*")
    install(FILES ${license} DESTINATION "${CMAKE_INSTALL_DATADIR}/kitepp/licenses/${dependency}")
endforeach()
install(FILES cmake/KiteppDependencies.cmake docs/runtime_contract.md docs/building.md
    docs/dependencies.md docs/release-notes.md
    DESTINATION "${CMAKE_INSTALL_DATADIR}/kitepp")
