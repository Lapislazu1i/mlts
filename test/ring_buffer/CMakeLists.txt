file(GLOB ring_buffer_test_src_files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(ring_buffer_test
    ${ring_buffer_test_src_files}
)
target_link_libraries(ring_buffer_test PRIVATE
    GTest::gtest_main
)