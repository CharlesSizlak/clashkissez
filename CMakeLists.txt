# set the project name and version
cmake_minimum_required(VERSION 3.16)

project(Clashkissez VERSION 0.3)

add_executable(chess_server
    "${CMAKE_CURRENT_LIST_DIR}/src/server.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/hash.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/loop.c"
    )

target_include_directories(chess_server PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/deps/klib"
    "${CMAKE_CURRENT_LIST_DIR}/include/generated"
    )

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)

target_compile_definitions(chess_server PRIVATE -D_GNU_SOURCE)

target_link_libraries(chess_server PUBLIC
    flatcc
    flatccrt
    rt
    )
