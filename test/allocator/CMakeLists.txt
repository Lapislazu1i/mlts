file(GLOB allocator_test_src_files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(allocator_test
    ${allocator_test_src_files}
)
target_link_libraries(allocator_test PRIVATE
    GTest::gtest_main
)