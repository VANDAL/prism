######################
# Shadow Memory Test #
######################
set (SOURCES )
add_executable(shadow_memory_test ShadMemTest.cpp ${SOURCES})
target_link_libraries(shadow_memory_test pthread rt)
add_test(shadow_memory_test shadow_memory_test)
