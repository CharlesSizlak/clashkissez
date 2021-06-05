# set the project name and version
cmake_minimum_required(VERSION 3.16)
project(Clashkissez VERSION 0.3)

# set(CMAKE_C_STANDARD 17)
# set(CMAKE_C_STANDARD_REQUIRED True)

add_library(loop STATIC
    "${CMAKE_CURRENT_LIST_DIR}/src/loop.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/hash.c"
    )

target_include_directories(loop PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/deps/klib"
    )

add_library(time_heap STATIC 
    "${CMAKE_CURRENT_LIST_DIR}/src/time_heap.c"
    )

target_include_directories(time_heap PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/deps/klib"
    )

add_executable(chess_server
    "${CMAKE_CURRENT_LIST_DIR}/src/server.c"
    )

target_include_directories(chess_server PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/include/generated"
    )

target_compile_definitions(chess_server PRIVATE -D_GNU_SOURCE)

target_link_libraries(chess_server PUBLIC
    flatcc
    flatccrt
    rt
    loop
    time_heap
    )

add_executable(loop_test "${CMAKE_CURRENT_LIST_DIR}/test/loop_test.c")

target_link_libraries(loop_test loop)

add_executable(time_heap_test "${CMAKE_CURRENT_LIST_DIR}/test/time_heap_test.c")

target_link_libraries(time_heap_test time_heap)