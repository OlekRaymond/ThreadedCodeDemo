
cmake_minimum_required(VERSION 3.10...3.16)
project(benchmarking)

option(ENABLE_PROFILING "Enable profiling flags" OFF)

find_package(benchmark QUIET)
find_package(gtest QUIET)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)

if (NOT gtest_FOUND)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

if(NOT benchmark_FOUND)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.4
    )
    FetchContent_MakeAvailable(benchmark)
endif()

set(SRC_FILES 
    ComputedGotos.cpp 
    main2.cpp
    Switch.cpp
)

if(ENABLE_PROFILING)
    message(STATUS "Profiling enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()

add_executable(benchmark_demo ${SRC_FILES})
if (ENABLE_ASAN)
    target_compile_options(benchmark_demo PRIVATE -fsanitize=address -g -O0)
    target_link_options(benchmark_demo PRIVATE -fsanitize=address)
endif()

if (MSVC)
    target_compile_options(benchmark_demo PRIVATE /W4 /WX)
else()
    target_compile_options(benchmark_demo PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

target_include_directories(benchmark_demo PRIVATE . )
target_link_libraries(benchmark_demo PRIVATE benchmark::benchmark GTest::gtest pthread)


include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    if( supported )
        message(STATUS "IPO / LTO enabled")
        set_property(TARGET benchmark_demo PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        target_compile_options(benchmark_demo PRIVATE -march=native -mtune=native)
    else()
        message(STATUS "IPO / LTO not supported: <${error}>")
    endif()
endif()