file(GLOB atomic_ring_buffer_test_src_files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(atomic_ring_buffer_test
    ${atomic_ring_buffer_test_src_files}
)
target_link_libraries(atomic_ring_buffer_test PRIVATE
    GTest::gtest_main
)