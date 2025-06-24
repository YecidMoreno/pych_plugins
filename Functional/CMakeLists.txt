add_library(Functional SHARED Functional.cpp)
target_link_libraries(Functional PUBLIC pych_core::core
    pych_core::reqs)