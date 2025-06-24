cmake_minimum_required(VERSION 3.16)
project(plugin_build)
set(CMAKE_CXX_STANDARD 17)

if(DEFINED TARGET_MACHINE)
    string(TOLOWER "${TARGET_MACHINE}" TARGET_MACHINE)
else()
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -dumpmachine
        OUTPUT_VARIABLE COMPILER_TRIPLE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(TOLOWER "${COMPILER_TRIPLE}" TARGET_MACHINE)
endif()

set(RELEASE_DIR "${CMAKE_SOURCE_DIR}/release/${TARGET_MACHINE}")
file(MAKE_DIRECTORY "${RELEASE_DIR}")


find_package(pych_core REQUIRED PATHS /core/release/${TARGET_MACHINE} ~/git/pych/release/${TARGET_MACHINE})

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(PYCH_INSTALLED_TARGETS)

add_subdirectory(ScienceMode3)
list(APPEND PYCH_INSTALLED_TARGETS ScienceMode3 Channel_sm3)

add_subdirectory(Functional)
list(APPEND PYCH_INSTALLED_TARGETS Functional)


install(TARGETS ${PYCH_INSTALLED_TARGETS} 
    LIBRARY DESTINATION plugins
    ARCHIVE DESTINATION plugins
)