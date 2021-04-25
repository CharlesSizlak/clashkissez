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
gcc server.c loop.c hash.c -I deps/klib -I include/generated -I include -lflatcc -lflatccrt -lrt -o chess_server
