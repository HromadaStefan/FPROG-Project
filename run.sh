#!/bin/bash

# Set the source file name
SOURCE_FILE="Fileread.cpp"

# Ensure the source file exists
if [ ! -f "$SOURCE_FILE" ]; then
    echo "Error: Source file not found: $SOURCE_FILE"
    exit 1
fi
set -x

echo "Running test for file: $SOURCE_FILE"

# Example for compiling and running a C++ file
clang -std=c++17 -lstdc++ -lm -Iinclude/ "$SOURCE_FILE" -Wall -Wextra -Werror -pthread -o out/ranges

# Run the compiled program
./out/ranges