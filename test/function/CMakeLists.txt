file(GLOB function_src_files "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(function
    ${function_src_files}
)
target_link_libraries(function PRIVATE
    GTest::gtest_main
)