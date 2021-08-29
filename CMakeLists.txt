# set the project name and version
cmake_minimum_required(VERSION 3.16)
project(Clashkissez VERSION 0.3)

# set(CMAKE_C_STANDARD 17)
# set(CMAKE_C_STANDARD_REQUIRED True)

find_package (mongoc-1.0 1.7 REQUIRED)

find_library(SSL_LIB libssl.so REQUIRED)
find_library(CRYPTO_LIB libcrypto.so REQUIRED)


add_library(loop STATIC
        "${CMAKE_CURRENT_LIST_DIR}/src/loop.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/hash.c"
    )

target_include_directories(loop PUBLIC 
        "${CMAKE_CURRENT_LIST_DIR}/include"
        "${CMAKE_CURRENT_LIST_DIR}/deps/klib"
    )

add_library(random STATIC
    "${CMAKE_CURRENT_LIST_DIR}/src/random.c"
)

target_include_directories(random PUBLIC 
        "${CMAKE_CURRENT_LIST_DIR}/include"
    )

add_library(time_heap STATIC 
        "${CMAKE_CURRENT_LIST_DIR}/src/time_heap.c"
    )

target_include_directories(time_heap PUBLIC 
        "${CMAKE_CURRENT_LIST_DIR}/include"
        "${CMAKE_CURRENT_LIST_DIR}/deps/klib"
    )

add_library(queue STATIC
        "${CMAKE_CURRENT_LIST_DIR}/src/queue.c"
    )

target_include_directories(queue PUBLIC 
        "${CMAKE_CURRENT_LIST_DIR}/include"
    )

add_library(serialib_chess STATIC
        "${CMAKE_CURRENT_LIST_DIR}/generated/chess.c"
    )

target_include_directories(serialib_chess PUBLIC 
        "${CMAKE_CURRENT_LIST_DIR}/generated"
    )

target_link_libraries(loop PUBLIC time_heap queue pthread)

add_executable(chess_server
        "${CMAKE_CURRENT_LIST_DIR}/src/server.c"
    )

target_include_directories(chess_server PUBLIC 
        "${CMAKE_CURRENT_LIST_DIR}/include"
    )

target_compile_definitions(chess_server PRIVATE -D_GNU_SOURCE)

target_link_libraries(chess_server
        PUBLIC
        serialib_chess
        rt
        loop
        queue
        time_heap
        pthread
        random
        ${SSL_LIB}
        ${CRYPTO_LIB}
        PRIVATE
        mongo::mongoc_shared
    )

add_executable(loop_test "${CMAKE_CURRENT_LIST_DIR}/test/loop_test.c")

target_link_libraries(loop_test loop)

add_executable(time_heap_test "${CMAKE_CURRENT_LIST_DIR}/test/time_heap_test.c")

target_link_libraries(time_heap_test time_heap)