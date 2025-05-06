file(GLOB cache_object_test_src_files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(cache_object_test
    ${cache_object_test_src_files}
)
target_link_libraries(cache_object_test PRIVATE
    GTest::gtest_main
)