cmake_minimum_required(VERSION 3.14)
project(ThreadPool VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CORTEX_BUILD_TESTS      "Build unit tests"   ON)
option(CORTEX_BUILD_BENCHMARKS "Build benchmarks"   ON)

# ── Compiler flags ─────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /EHsc)
    add_compile_definitions($<$<CONFIG:Release>:NDEBUG>)
else()
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
    add_compile_options(-Wpedantic)
    add_compile_options($<$<CONFIG:Debug>:-g>)
    add_compile_options($<$<CONFIG:Debug>:-O0>)
    add_compile_options($<$<CONFIG:Release>:-O3>)
    add_compile_definitions($<$<CONFIG:Release>:NDEBUG>)
endif()

find_package(Threads REQUIRED)

enable_testing()

# ──────────────────────────────────────────────────────────────
# 📦 Library Target
# ──────────────────────────────────────────────────────────────
add_library(thread_pool_lib
    src/task_queue.cpp
    src/thread_pool.cpp
    src/worker.cpp
)

# Alias so both add_subdirectory and find_package users get Cortex::thread_pool_lib
add_library(Cortex::thread_pool_lib ALIAS thread_pool_lib)

target_include_directories(thread_pool_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(thread_pool_lib PUBLIC Threads::Threads)

if(MSVC)
    target_compile_options(thread_pool_lib PRIVATE /wd4324)
endif()

# ──────────────────────────────────────────────────────────────
# 🧪 GoogleTest Setup
# ──────────────────────────────────────────────────────────────
if(CORTEX_BUILD_TESTS)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)

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
        if(MSVC)
            target_compile_options(${test_name} PRIVATE /wd4324 /wd4267)
        endif()
        gtest_discover_tests(${test_name}
            DISCOVERY_TIMEOUT 60
            PROPERTIES TIMEOUT 120
        )
    endforeach()
endif()

# ──────────────────────────────────────────────────────────────
# 📊 Google Benchmark Setup
# ──────────────────────────────────────────────────────────────
if(CORTEX_BUILD_BENCHMARKS)
    include(FetchContent)

    FetchContent_Declare(
        googlebenchmark
        URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    set(BENCHMARK_ENABLE_TESTING     OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_INSTALL_DOCS       OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_USE_LIBCXX         OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googlebenchmark)

    add_executable(bench_thread_pool benchmarks/bench_thread_pool.cpp)
    target_link_libraries(bench_thread_pool PRIVATE
        thread_pool_lib
        benchmark::benchmark
        Threads::Threads
    )
    if(MSVC)
        target_compile_options(bench_thread_pool PRIVATE
            /wd4324
            /wd4267
            /wd4996
        )
    endif()
endif()

# ──────────────────────────────────────────────────────────────
# ✅ Install Rules
# ──────────────────────────────────────────────────────────────
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS thread_pool_lib
    EXPORT CortexThreadpoolTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cortex
)

install(EXPORT CortexThreadpoolTargets
    FILE CortexThreadpoolTargets.cmake
    NAMESPACE Cortex::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CortexThreadpool
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/CortexThreadpoolConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CortexThreadpoolConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/CortexThreadpoolConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CortexThreadpool
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/CortexThreadpoolConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/CortexThreadpoolConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CortexThreadpool
)