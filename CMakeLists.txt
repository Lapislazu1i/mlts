cmake_minimum_required(VERSION 3.20)

# include("D:/vcpkg/scripts/buildsystems/vcpkg.cmake")
project(mlts)

file(GLOB mltl_include_files "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h*")

message(STATUS "mlts include files:")
foreach(file ${mltl_include_files})
	message(STATUS ${file})
endforeach()

add_library(mlts INTERFACE ${mltl_include_files})

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/vendor/googletest")
set(BENCHMARK_ENABLE_TESTING OFF)
# set(BENCHMARK_USE_BUNDLED_GTEST OFF)
add_definitions(
	-DWIN32
	-D_WINDOWS
	-D_CRT_SECURE_NO_WARNINGS
	-DHAVE_STD_REGEX
	-DHAVE_STEADY_CLOCK
	-DTEST_BENCHMARK_LIBRARY_HAS_NO_ASSERTIONS
	-DBENCHMARK_STATIC_DEFINE
)

enable_testing()
include(GoogleTest)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")


