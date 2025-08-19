add_library(TCPClient SHARED TCPClient.cpp)
target_link_libraries(TCPClient PUBLIC pych_core::core
    pych_core::reqs)

    add_library(TCPDecode SHARED TCPDecode.cpp)
target_link_libraries(TCPDecode PUBLIC pych_core::core
    pych_core::reqs)