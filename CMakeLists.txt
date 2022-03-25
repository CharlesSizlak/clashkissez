# set the project name and version
cmake_minimum_required(VERSION 3.21)
project(Clashkissez VERSION 0.3)

SET(CMAKE_C_COMPILER "/usr/bin/clang")
# set(CMAKE_C_STANDARD 17)
# set(CMAKE_C_STANDARD_REQUIRED True)

find_package(mongoc-1.0 1.7 REQUIRED)
find_library(SSL_LIB libssl.so REQUIRED)
find_library(CRYPTO_LIB libcrypto.so REQUIRED)


set(
    PROJECT_COMPILE_OPTIONS
    -Wall -Wextra -Wpedantic
    -Wno-gnu-zero-variadic-macro-arguments
    -Wno-gnu-empty-struct
    -Wno-unused-parameter
)


add_library(serialib_chess STATIC
    "${CMAKE_CURRENT_LIST_DIR}/generated/chess.c"
)
target_include_directories(serialib_chess PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/generated"
)
target_compile_options(serialib_chess PRIVATE ${PROJECT_COMPILE_OPTIONS})
set_property(TARGET serialib_chess PROPERTY C_STANDARD 23)


add_executable(chess_server
    "${CMAKE_CURRENT_LIST_DIR}/src/vector.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/server.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/random.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/loop.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/hash.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/time_heap.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/queue.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/contexts.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/database.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/models.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/friend_request_accepted_subscribe_handler.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/friend_request_subscribe_handler.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/game_invite_response_subscribe_handler.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/game_invite_handler.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/game_invite_subscribe_handler.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/game_subscribe_handler.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/handlers.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/login_request_handler.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/register_request_handler.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/security.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/message_handler.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/resolve_game_invite_handler.c"

    )
target_include_directories(chess_server PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/deps/klib"
)
target_compile_definitions(chess_server PRIVATE -D_GNU_SOURCE)
target_compile_options(chess_server PRIVATE ${PROJECT_COMPILE_OPTIONS})
set_property(TARGET chess_server PROPERTY C_STANDARD 23)
target_link_libraries(chess_server
    PUBLIC
    serialib_chess
    rt
    pthread
    ${SSL_LIB}
    ${CRYPTO_LIB}
    PRIVATE
    mongo::mongoc_shared
)


add_executable(loop_test
    "${CMAKE_CURRENT_LIST_DIR}/test/loop_test.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/loop.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/time_heap.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/queue.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/hash.c"
)
target_link_libraries(loop_test pthread)
target_include_directories(loop_test PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/deps/klib"
)
target_compile_options(loop_test PRIVATE ${PROJECT_COMPILE_OPTIONS})
set_property(TARGET loop_test PROPERTY C_STANDARD 23)


add_executable(time_heap_test
    "${CMAKE_CURRENT_LIST_DIR}/test/time_heap_test.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/time_heap.c"
)
target_include_directories(time_heap_test PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/deps/klib"
)
target_compile_options(time_heap_test PRIVATE ${PROJECT_COMPILE_OPTIONS})
set_property(TARGET time_heap_test PROPERTY C_STANDARD 23)

add_executable(timer_test
    "${CMAKE_CURRENT_LIST_DIR}/test/timer_test.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/time_heap.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/queue.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/hash.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/loop.c"
)
target_include_directories(timer_test PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/deps/klib"
)
target_link_libraries(timer_test PUBLIC pthread)

target_compile_options(timer_test PRIVATE ${PROJECT_COMPILE_OPTIONS})
set_property(TARGET timer_test PROPERTY C_STANDARD 23)
