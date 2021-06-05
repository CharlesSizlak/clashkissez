#!/bin/bash

# Make a generated include directory
mkdir -p include/generated
pushd include/generated >/dev/null

# Compile the flat buffer schema
flatcc -a ../../chess.fbs

# Return to top level
popd >/dev/null

# Generate the python libraries
mkdir -p pychess
pushd pychess >/dev/null
flatc --python ../chess.fbs
popd >/dev/null

# Build the server
#gcc -std=c17 -D_GNU_SOURCE src/server.c src/loop.c src/hash.c -I deps/klib -I include/generated -I include -lflatcc -lflatccrt -lrt -o chess_server
mkdir -p build
cd build
cmake ..
make
cd ..