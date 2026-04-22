cmake_minimum_required(VERSION 3.14)
project(ThreadPool VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

find_package(Threads REQUIRED)

# ──────────────────────────────────────────────────────────────
# 📦 Library Target
# ──────────────────────────────────────────────────────────────
add_library(thread_pool_lib
    src/task_queue.cpp
    src/thread_pool.cpp
    src/worker.cpp
)

target_include_directories(thread_pool_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(thread_pool_lib PUBLIC Threads::Threads)

# ──────────────────────────────────────────────────────────────
# 🧪 GoogleTest Setup (for unit tests)
# ──────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP true
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# Test executables
set(TEST_TARGETS
    test_task_queue
    test_thread_pool
    test_priority_queue
    test_timeout
    test_advanced
)

foreach(test_name ${TEST_TARGETS})
    add_executable(${test_name} tests/${test_name}.cpp)
    target_link_libraries(${test_name}
        PRIVATE
            thread_pool_lib
            gtest_main
            Threads::Threads
    )
    gtest_discover_tests(${test_name})
endforeach()

# ──────────────────────────────────────────────────────────────
# 📊 Google Benchmark Setup (for performance tests)
# ──────────────────────────────────────────────────────────────
FetchContent_Declare(
    googlebenchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.9.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP true
)

# 🔧 Skip all internal tests & docs (only needed for library usage)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googlebenchmark)

add_executable(bench_thread_pool benchmarks/bench_thread_pool.cpp)
target_link_libraries(bench_thread_pool PRIVATE 
    thread_pool_lib 
    benchmark::benchmark 
    Threads::Threads
)
# ──────────────────────────────────────────────────────────────
# ✅ Enable Testing & Install Rules
# ──────────────────────────────────────────────────────────────
enable_testing()

include(GNUInstallDirs)

install(TARGETS thread_pool_lib
    EXPORT CortexThreadpoolTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cortex
)

install(EXPORT CortexThreadpoolTargets
    FILE CortexThreadpoolTargets.cmake
    NAMESPACE Cortex::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CortexThreadpool
)