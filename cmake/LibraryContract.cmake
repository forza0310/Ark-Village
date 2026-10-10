# Shared source identity and completed-artifact inventory are distinct: configuring
# or partially rebuilding a library must never certify stale DLL bytes as current.
function(ark_publish_library_contract)
    set(contract_dir "${CMAKE_BINARY_DIR}/contract")
    set(inputs "${PROJECT_SOURCE_DIR}/CMakeLists.txt")
    foreach(file IN ITEMS cmake/LibraryContract.cmake cmake/ProductLibraries.cmake
            cmake/WorldSimulation.cmake cmake/WorldSimulationSources.cmake
            cmake/SharedRuntime.cmake scripts/shared_library_contract.mjs
            scripts/compile_desktop_glyphs.mjs scripts/simulation/compile_startup.mjs
            scripts/simulation/compile_startup_world.mjs scripts/simulation/compile_persistence_identity.mjs
            src/app/bootstrap/shared_library_identity.cpp)
        list(APPEND inputs "${PROJECT_SOURCE_DIR}/${file}")
    endforeach()
    set(artifacts)
    set(modules)
    set(settings "${CMAKE_CXX_COMPILER_ID}|${CMAKE_CXX_COMPILER_VERSION}|${CMAKE_SIZEOF_VOID_P}|${CMAKE_CXX_FLAGS}|${CMAKE_CXX_FLAGS_RELEASE}|${CMAKE_SHARED_LINKER_FLAGS}|${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
    foreach(target IN LISTS ARK_PRODUCT_LIBRARIES)
        if(MINGW)
            # One explicit dllexport otherwise disables MinGW's implicit export
            # of the existing unannotated C++ API. Preserve that API alongside
            # the stable C contract symbol; MSVC uses WINDOWS_EXPORT_ALL_SYMBOLS.
            target_link_options(${target} PRIVATE "LINKER:--export-all-symbols")
        endif()
        get_target_property(sources ${target} SOURCES)
        foreach(source IN LISTS sources)
            if(NOT IS_ABSOLUTE "${source}")
                set(source "${PROJECT_SOURCE_DIR}/${source}")
            endif()
            # Generated C++ is represented by the explicit generator/data inputs.
            if(NOT source MATCHES "^${CMAKE_BINARY_DIR}/")
                list(APPEND inputs "${source}")
            endif()
        endforeach()
        foreach(property IN ITEMS COMPILE_DEFINITIONS COMPILE_OPTIONS INCLUDE_DIRECTORIES LINK_LIBRARIES LINK_OPTIONS)
            get_target_property(value ${target} ${property})
            string(APPEND settings "|${target}:${property}=${value}")
        endforeach()
        list(APPEND artifacts "$<TARGET_FILE:${target}>")
        list(APPEND modules "$<TARGET_FILE_NAME:${target}>")
        if(WIN32)
            list(APPEND artifacts "$<TARGET_LINKER_FILE:${target}>")
        endif()
        target_sources(${target} PRIVATE "${PROJECT_SOURCE_DIR}/src/app/bootstrap/shared_library_identity.cpp")
        target_include_directories(${target} PRIVATE "${contract_dir}")
    endforeach()
    list(REMOVE_DUPLICATES inputs)
    list(SORT inputs)
    # CMake paths use forward slashes; JSON quoting also protects flag quotes.
    foreach(variable IN ITEMS inputs artifacts modules)
        set(json_${variable})
        foreach(value IN LISTS ${variable})
            string(REPLACE "\\" "\\\\" value "${value}")
            string(REPLACE "\"" "\\\"" value "${value}")
            list(APPEND json_${variable} "\"${value}\"")
        endforeach()
        list(JOIN json_${variable} "," json_${variable})
    endforeach()
    string(REPLACE "\\" "\\\\" settings "${settings}")
    string(REPLACE "\"" "\\\"" settings "${settings}")
    string(REPLACE "\n" "\\n" settings "${settings}")
    if(ARK_BUILD_DESKTOP)
        set(desktop_glyphs true)
    else()
        set(desktop_glyphs false)
    endif()
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/ArkLibraryInputs.json"
        CONTENT "{\"sources\":[${json_inputs}],\"artifacts\":[${json_artifacts}],\"modules\":[${json_modules}],\"desktopGlyphs\":${desktop_glyphs},\"settings\":\"${settings}\"}\n")
    add_custom_target(ark_library_contract_prepare
        COMMAND "${ARK_NODE}" "${PROJECT_SOURCE_DIR}/scripts/shared_library_contract.mjs"
            prepare "${PROJECT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}"
        BYPRODUCTS "${contract_dir}/ark_library_contract.hpp" VERBATIM)
    foreach(target IN LISTS ARK_PRODUCT_LIBRARIES)
        add_dependencies(${target} ark_library_contract_prepare)
    endforeach()
    add_custom_target(ark_library_contract_publish ALL
        COMMAND "${ARK_NODE}" "${PROJECT_SOURCE_DIR}/scripts/shared_library_contract.mjs"
            publish "${PROJECT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}"
        DEPENDS ${ARK_PRODUCT_LIBRARIES} VERBATIM)
endfunction()

function(ark_consume_library_contract)
    set(ARK_CONSUMER_CONTRACT_DIR "${CMAKE_BINARY_DIR}/contract" PARENT_SCOPE)
    execute_process(COMMAND "${ARK_NODE}" "${PROJECT_SOURCE_DIR}/scripts/shared_library_contract.mjs"
        check "${PROJECT_SOURCE_DIR}" "${ARK_COMMON_LIBRARY_DIR}"
        "${CMAKE_BINARY_DIR}/contract/ark_library_contract.hpp"
        RESULT_VARIABLE result ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${error}")
    endif()
    add_custom_target(ark_library_contract_check
        COMMAND "${ARK_NODE}" "${PROJECT_SOURCE_DIR}/scripts/shared_library_contract.mjs"
            check "${PROJECT_SOURCE_DIR}" "${ARK_COMMON_LIBRARY_DIR}"
            "${CMAKE_BINARY_DIR}/contract/ark_library_contract.hpp"
        BYPRODUCTS "${CMAKE_BINARY_DIR}/contract/ark_library_contract.hpp" VERBATIM)
endfunction()
