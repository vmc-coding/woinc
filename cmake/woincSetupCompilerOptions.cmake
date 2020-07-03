macro(woincSetupCompilerOptions target)
    target_compile_features(${target} PUBLIC cxx_std_14)

    if(CMAKE_BUILD_TYPE STREQUAL "dev_release")
        set(CMAKE_AR "gcc-ar")
        set(CMAKE_RANLIB "gcc-ranlib")
        target_compile_definitions(${target} PRIVATE NDEBUG)
        target_compile_options(${target} PRIVATE -g -fdiagnostics-color=always)
        target_compile_options(${target} PRIVATE
            -Os -Wl,-Map,woinc.map -fuse-ld=gold -fuse-linker-plugin
            -flto -Wl,-flto -fvisibility-inlines-hidden)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Werror)
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "dev_debug")
        target_compile_options(${target} PRIVATE -g -fdiagnostics-color=always)
        target_compile_options(${target} PRIVATE
            -Wall
            -Wcast-align
            -Wconversion
            -Wdouble-promotion
            -Wextra
            -Wnon-virtual-dtor
            -Wnull-dereference
            -Wold-style-cast
            -Woverloaded-virtual
            -Wpedantic
            -Wshadow
            -Wsign-conversion
            -Wunused)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE
                -Wduplicated-cond
                -Wlogical-op)
                #-Wuseless-cast)
            if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "7")
                target_compile_options(${target} PRIVATE -Wduplicated-branches)
            endif()
        endif()
        #target_compile_definitions(${target} PRIVATE "WOINC_LOG_RPC_CONNECTION")
    endif()

    if(WOINC_ENABLE_SANITIZER)
        target_compile_options(${target} PRIVATE -fsanitize=address -fsanitize=undefined)
    endif()

    if(WOINC_ENABLE_COVERAGE)
        target_compile_options(${target} PRIVATE --coverage -O0 -g)
        if(NOT "${target}" STREQUAL "woinc_ui_common")
            target_link_libraries(${target} PRIVATE --coverage)
        endif()
    endif()
endmacro()
