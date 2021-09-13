# set the project name and version
cmake_minimum_required(VERSION 3.21)
project(Clashkissez VERSION 0.3)

SET(CMAKE_C_COMPILER "/usr/bin/clang")
# set(CMAKE_C_STANDARD 17)
# set(CMAKE_C_STANDARD_REQUIRED True)

find_package(mongoc-1.0 1.7 REQUIRED)
find_library(SSL_LIB libssl.so REQUIRED)
find_library(CRYPTO_LIB libcrypto.so REQUIRED)


add_library(random STATIC
    "${CMAKE_CURRENT_LIST_DIR}/src/random.c"
)
target_include_directories(random PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/include"
)
set(
    PROJECT_COMPILE_OPTIONS
    -Wall -Wextra -Wpedantic
    -Wno-gnu-zero-variadic-macro-arguments
    -Wno-gnu-empty-struct
    -Wno-unused-parameter
)
target_compile_options(random PRIVATE ${PROJECT_COMPILE_OPTIONS})
set_property(TARGET random PROPERTY C_STANDARD 23)


add_library(vector STATIC
    "${CMAKE_CURRENT_LIST_DIR}/src/vector.c"
)
target_include_directories(vector PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/include"
)
target_compile_options(vector PRIVATE ${PROJECT_COMPILE_OPTIONS})
set_property(TARGET vector PROPERTY C_STANDARD 23)


add_library(time_heap STATIC 
    "${CMAKE_CURRENT_LIST_DIR}/src/time_heap.c"
)
target_include_directories(time_heap PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/deps/klib"
)
target_compile_options(time_heap PRIVATE ${PROJECT_COMPILE_OPTIONS})
set_property(TARGET time_heap PROPERTY C_STANDARD 23)


add_library(queue STATIC
    "${CMAKE_CURRENT_LIST_DIR}/src/queue.c"
)
target_include_directories(queue PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/include"
)
target_compile_options(queue PRIVATE ${PROJECT_COMPILE_OPTIONS})
set_property(TARGET queue PROPERTY C_STANDARD 23)


add_library(serialib_chess STATIC
    "${CMAKE_CURRENT_LIST_DIR}/generated/chess.c"
)
target_include_directories(serialib_chess PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/generated"
)
target_compile_options(serialib_chess PRIVATE ${PROJECT_COMPILE_OPTIONS})
set_property(TARGET serialib_chess PROPERTY C_STANDARD 23)


add_library(loop STATIC
    "${CMAKE_CURRENT_LIST_DIR}/src/loop.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/hash.c"
)
target_include_directories(loop PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/deps/klib"
)
target_compile_options(loop PRIVATE ${PROJECT_COMPILE_OPTIONS})
target_link_libraries(loop PUBLIC time_heap queue pthread)
set_property(TARGET loop PROPERTY C_STANDARD 23)


add_executable(chess_server
    "${CMAKE_CURRENT_LIST_DIR}/src/server.c"
)
target_include_directories(chess_server PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/include"
)
target_compile_definitions(chess_server PRIVATE -D_GNU_SOURCE)
target_compile_options(chess_server PRIVATE ${PROJECT_COMPILE_OPTIONS})
set_property(TARGET chess_server PROPERTY C_STANDARD 23)
target_link_libraries(chess_server
    PUBLIC
    serialib_chess
    rt
    loop
    queue
    time_heap
    pthread
    random
    vector
    ${SSL_LIB}
    ${CRYPTO_LIB}
    PRIVATE
    mongo::mongoc_shared
)


add_executable(loop_test "${CMAKE_CURRENT_LIST_DIR}/test/loop_test.c")
target_link_libraries(loop_test loop)
target_compile_options(loop_test PRIVATE ${PROJECT_COMPILE_OPTIONS})
set_property(TARGET loop_test PROPERTY C_STANDARD 23)


add_executable(time_heap_test "${CMAKE_CURRENT_LIST_DIR}/test/time_heap_test.c")
target_link_libraries(time_heap_test time_heap)
target_compile_options(time_heap_test PRIVATE ${PROJECT_COMPILE_OPTIONS})
set_property(TARGET time_heap_test PROPERTY C_STANDARD 23)
