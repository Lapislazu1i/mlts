file(GLOB lock_free_queue_test_src_files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(lock_free_queue_test
    ${lock_free_queue_test_src_files}
)
target_link_libraries(lock_free_queue_test PRIVATE
    GTest::gtest_main
)