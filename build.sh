#!/bin/bash

BUILD_DIR='build'
BINARY_DIR='binaries'
MODE='Release'

# Cleanup generated if it already exists
rm -rf generated
mkdir -p generated

# Compile the serialib schema
python3 deps/serialib/generate.py chess.schema \
    --c-source generated/chess.c \
    --c-header generated/chess.h \
    --python generated/chess.py

# Build the server
mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake -DCMAKE_BUILD_TYPE=$MODE ..
make
cd ..

# Make binary dir
mkdir -p $BINARY_DIR

# Move generated chess.py into binary dir
cp generated/chess.py $BINARY_DIR

# Move C executables into binary dir
cp $BUILD_DIR/chess_server $BINARY_DIR

# Move python source into binary dir
cp src/*.py $BINARY_DIR
