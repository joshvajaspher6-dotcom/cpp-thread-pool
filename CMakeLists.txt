cmake_minimum_required(VERSION 3.14)
project(ThreadPool VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

include_directories(include)

add_library(thread_pool_lib
            src/task_queue.cpp
            src/thread_pool.cpp
            src/worker.cpp
        )


target_link_libraries(thread_pool_lib pthread)        
target_include_directories(thread_pool_lib PUBLIC include)

add_executable(demo src/main.cpp)
target_link_libraries(demo thread_pool_lib)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(googletest)

add_executable(test_task_queue tests/test_task_queue.cpp)
add_executable(test_thread_pool tests/test_thread_pool.cpp)

foreach(t test_task_queue test_thread_pool)
    target_link_libraries(${t}
        thread_pool_lib
        gtest_main
        pthread
    )
endforeach()

enable_testing()
add_test(NAME TaskQueueTest  COMMAND test_task_queue)
add_test(NAME ThreadPoolTest COMMAND test_thread_pool)