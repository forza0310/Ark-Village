# Development and test executables reuse one set of project and dependency DLLs.
# Player packaging selects only the Release game's transitive runtime dependencies.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
if(UNIX AND NOT APPLE)
    set(CMAKE_BUILD_RPATH "$ORIGIN")
endif()

set(ARK_SHARED_RUNTIME_DIRECTORY "${PROJECT_SOURCE_DIR}/build/bin")
if(CMAKE_CONFIGURATION_TYPES)
    string(APPEND ARK_SHARED_RUNTIME_DIRECTORY "/$<CONFIG>")
endif()
add_custom_target(ark_shared_runtime ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ARK_SHARED_RUNTIME_DIRECTORY}")

function(ark_use_shared_runtime target)
    add_dependencies(${target} ark_shared_runtime)
    if(NOT ARK_LIBRARIES_ONLY)
        get_target_property(kind ${target} TYPE)
        if(kind STREQUAL "EXECUTABLE")
            set_target_properties(${target} PROPERTIES OUTPUT_NAME "${target}-${ARK_BUILD_PROFILE}")
        endif()
    endif()
endfunction()

function(ark_copy_raylib_runtime)
    if(WIN32)
        find_file(ark_raylib_runtime NAMES raylib.dll libraylib.dll
            HINTS "${RAYLIB_PREFIX}/bin" "${RAYLIB_PREFIX}/lib"
            NO_DEFAULT_PATH NO_CACHE REQUIRED)
        add_custom_command(TARGET ark_shared_runtime POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ark_raylib_runtime}"
                "${ARK_SHARED_RUNTIME_DIRECTORY}"
            VERBATIM)
    endif()
endfunction()

if(MINGW)
    foreach(flag_var CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_CXX_FLAGS)
        foreach(suffix "" _DEBUG _RELEASE _RELWITHDEBINFO _MINSIZEREL)
            if("${${flag_var}${suffix}}" MATCHES "(^|[ \t])-static(-libstdc\\+\\+|-libgcc)?($|[ \t])")
                message(FATAL_ERROR
                    "Shared builds cannot use static runtime flags in ${flag_var}${suffix}. Configure a new build directory or clear that cached flag.")
            endif()
        endforeach()
    endforeach()
    # LLVM-MinGW keeps target runtime DLLs beside the target import-library directory.
    # Asking the compiler avoids accidentally copying its host-architecture DLLs.
    execute_process(COMMAND "${CMAKE_CXX_COMPILER}" "-print-file-name=libc++.dll.a"
        OUTPUT_VARIABLE ark_libcxx_import OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    set(ark_runtime_dlls)
    if(IS_ABSOLUTE "${ark_libcxx_import}" AND EXISTS "${ark_libcxx_import}")
        get_filename_component(ark_target_lib_dir "${ark_libcxx_import}" DIRECTORY)
        get_filename_component(ark_target_dir "${ark_target_lib_dir}" DIRECTORY)
        foreach(name libc++.dll libunwind.dll libwinpthread-1.dll)
            set(dll "${ark_target_dir}/bin/${name}")
            if(NOT EXISTS "${dll}")
                message(FATAL_ERROR "Missing target runtime DLL: ${dll}")
            endif()
            list(APPEND ark_runtime_dlls "${dll}")
        endforeach()
    else()
        # GCC-MinGW can resolve its runtime DLLs directly through its target search paths.
        foreach(name libstdc++-6.dll libgcc_s_seh-1.dll libgcc_s_dw2-1.dll libgcc_s_sjlj-1.dll
                     libwinpthread-1.dll)
            execute_process(COMMAND "${CMAKE_CXX_COMPILER}" "-print-file-name=${name}"
                OUTPUT_VARIABLE dll OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
            if(IS_ABSOLUTE "${dll}" AND EXISTS "${dll}")
                list(APPEND ark_runtime_dlls "${dll}")
            endif()
        endforeach()
    endif()
    if(NOT ark_runtime_dlls)
        message(FATAL_ERROR "Cannot locate the selected MinGW compiler's shared C++ runtime")
    endif()
    if(ARK_LIBRARIES_ONLY)
        add_custom_command(TARGET ark_shared_runtime POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ark_runtime_dlls}
                "${ARK_SHARED_RUNTIME_DIRECTORY}"
            VERBATIM)
    endif()
endif()
