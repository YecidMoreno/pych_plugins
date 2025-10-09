add_library(SimpleImpedance SHARED SimpleImpedance.cpp)
target_link_libraries(SimpleImpedance PUBLIC pych_core::core
    pych_core::reqs)

add_library(SimpleCurrentMode SHARED SimpleCurrentMode.cpp)
target_link_libraries(SimpleCurrentMode PUBLIC pych_core::core
    pych_core::reqs)