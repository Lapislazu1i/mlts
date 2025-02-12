file(GLOB lambda_box_src_files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(lambda_box
    ${lambda_box_src_files}
)
target_link_libraries(lambda_box PRIVATE
    GTest::gtest_main
)