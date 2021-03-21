#!/bin/bash

# Make a generated include directory
mkdir -p include/generated
cd include/generated

# Compile the flat buffer schema
flatcc -a ../../chess.fbs

# Return to top level
cd ../..

# Build the server
gcc server.c -I include/generated -I include -lflatcc -o chess_server
