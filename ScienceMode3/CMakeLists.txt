set(SM3_PATH "hasomed_precompiled_library/smpt_rm3_gcc_linux_x86_amd64_static")
set(SM3_INCLUDES ${SM3_PATH}/include/mid-level
    ${SM3_PATH}/include/low-level
    ${SM3_PATH}/include/general
    ${SM3_PATH}/lib)


add_library(ScienceMode3 SHARED ScienceMode3.cpp)
target_link_libraries(ScienceMode3 PUBLIC pych_core::core
    pych_core::reqs)

target_include_directories(ScienceMode3 PRIVATE ${SM3_INCLUDES})
target_link_directories(ScienceMode3 PRIVATE ${SM3_PATH}/lib hasomed_precompiled_library/smpt_rm3_gcc_linux_arm_aarch64_static/lib)
target_link_libraries(ScienceMode3 PRIVATE libsmpt.a smpt m)


add_library(Channel_sm3 SHARED Channel_sm3.cpp)
target_link_libraries(Channel_sm3 PUBLIC pych_core::core
    pych_core::reqs)

