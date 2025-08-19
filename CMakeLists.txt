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


find_package(pych_core REQUIRED PATHS /core /core/release/${TARGET_MACHINE})

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(PYCH_INSTALLED_TARGETS)

include_directories(".")

add_subdirectory(ScienceMode3)
list(APPEND PYCH_INSTALLED_TARGETS ScienceMode3 Channel_sm3)

add_subdirectory(Functional)
list(APPEND PYCH_INSTALLED_TARGETS Functional)

add_subdirectory(TCPClient)
list(APPEND PYCH_INSTALLED_TARGETS TCPClient)
list(APPEND PYCH_INSTALLED_TARGETS TCPDecode)

add_subdirectory(Logger)
list(APPEND PYCH_INSTALLED_TARGETS LogPrint)

add_subdirectory(DelsysBaseStation)
list(APPEND PYCH_INSTALLED_TARGETS DelsysBaseStation)


install(TARGETS ${PYCH_INSTALLED_TARGETS} 
    LIBRARY DESTINATION plugins
    ARCHIVE DESTINATION plugins
)

