#!/bin/bash
set -euo pipefail

# Cleanup generated if it already exists
if [[ ! generated/chess.c -nt chess.schema ]] || \
   [[ ! generated/chess.c -nt deps/serialib/generate.py ]]
then
    rm -rf generated
    mkdir -p generated

    # Compile the serialib schema
    python3 deps/serialib/generate.py chess.schema \
        --c-source generated/chess.c \
        --c-header generated/chess.h \
        --python generated/chess.py
fi
