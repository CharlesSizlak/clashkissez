#!/bin/bash

BUILD_DIR='debug'
BINARY_DIR='binaries'
MODE='Debug'

# Cleanup pychess if it already exists
rm -rf $BUILD_DIR/pychess

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
mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake -DCMAKE_BUILD_TYPE=$MODE ..
make
cd ..

# Make binary dir
mkdir -p $BINARY_DIR

# Move pychess into binary dir
cp -r pychess $BINARY_DIR

# Move C executes into binary dir
cp $BUILD_DIR/chess_server $BINARY_DIR

# Move python source into binary dir
cp src/*.py $BINARY_DIR
