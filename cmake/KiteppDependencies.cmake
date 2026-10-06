# Read-only verification of bundled dependency identities. No acquisition step.
function(kitepp_verify_dependency directory revision header digest)
    set(root "${CMAKE_CURRENT_SOURCE_DIR}/${directory}")
    if(NOT EXISTS "${root}/${header}")
        message(FATAL_ERROR "Missing ${directory}/${header}; initialize recorded submodules with git submodule update --init --recursive")
    endif()
    file(SHA256 "${root}/${header}" actual_digest)
    if(NOT actual_digest STREQUAL digest)
        message(FATAL_ERROR "Bundled ${directory}/${header} differs from its recorded identity; update and qualify the dependency lock explicitly")
    endif()
    if(EXISTS "${root}/.git")
        find_package(Git QUIET)
        if(Git_FOUND)
            execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${root}" rev-parse HEAD
                OUTPUT_VARIABLE actual_revision OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE git_result)
            execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${root}" diff --quiet HEAD --
                RESULT_VARIABLE dirty_result)
            if(NOT git_result EQUAL 0 OR NOT actual_revision STREQUAL revision OR
                NOT dirty_result EQUAL 0)
                message(FATAL_ERROR "${directory} is not the clean recorded revision ${revision}")
            endif()
        endif()
    endif()
endfunction()

kitepp_verify_dependency(include/cpp-httplib
    cf3693cb5cc0d39b0e6f4122ba89bda9edb5ac6b httplib.h
    dc1e4de3e0ef3a18f3a5818fab563eb3fa45d5d3bb1db527a8d389077ed5ee6c)
kitepp_verify_dependency(include/fmt
    1be298e1bd68957e4cd352e1f676f00e07dcfb57 include/fmt/format.h
    b95f7c5b45d3d93e7cd8e691ae3040282ebcfe2d9c390f2c19d15ffcfae80a9c)
kitepp_verify_dependency(include/rapidjson
    1ce516e50bec548eb3273e5b8563d97a18ba233c include/rapidjson/document.h
    9803a673462349b352581e892217cd750957a783a286c8474d5822110d912b69)
kitepp_verify_dependency(include/rapidcsv
    cbd8a0a937b249cc07e2db3bfa9cd2cc1689708f src/rapidcsv.h
    6521f1d20a0cbfc80da767e58c9e071f1b81e6f789f61c7bc0336d3436ad9662)
kitepp_verify_dependency(include/PicoSHA2
    27fcf6979298949e8a462e16d09a0351c18fcaf2 picosha2.h
    8f183eaae529cd9d6a3d4843c7559e2a3e3d68b6caaa223e7c24c3c899b3d988)
