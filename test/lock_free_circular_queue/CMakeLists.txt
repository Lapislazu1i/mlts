file(GLOB lock_free_circular_queue_src_files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(lock_free_circular_queue
    ${lock_free_circular_queue_src_files}
)
target_link_libraries(lock_free_circular_queue PRIVATE
    GTest::gtest_main
)