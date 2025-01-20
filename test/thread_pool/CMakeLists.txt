file(GLOB thread_pool_test_src_files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(thread_pool_test
    ${thread_pool_test_src_files}
)
target_link_libraries(thread_pool_test PRIVATE
    GTest::gtest_main
)