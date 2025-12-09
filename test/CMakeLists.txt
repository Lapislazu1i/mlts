
set(CMAKE_CXX_STANDARD 20)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include")

set(mlts_test_pch_file "${CMAKE_CURRENT_SOURCE_DIR}/include/pch.h")


file(GLOB mlts_test_src_files "${CMAKE_CURRENT_SOURCE_DIR}/src")


add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/allocator")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/lambda_box")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/lock_free_circular_queue")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/lock_free_queue")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/thread_pool")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ring_buffer")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/atomic_ring_buffer")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/cache_object")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/function")




