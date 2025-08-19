add_library(DelsysBaseStation SHARED DelsysBaseStation.cpp)
target_link_libraries(DelsysBaseStation PUBLIC pych_core::core
    pych_core::reqs)