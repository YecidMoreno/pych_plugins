add_library(LogPrint SHARED LogPrint.cpp)
target_link_libraries(LogPrint PUBLIC pych_core::core
    pych_core::reqs)
