#!/bin/bash

# WASM Workings: This script is executed by the Developer/Build System. It tells the Emscripten Compiler (emcc) to take the C++ files (huffman_coding_algorithm.cpp and wasm_bindings.cpp) and headers and compile them into WebAssembly. 
# It generates two files: 1. huffman_coding.wasm (The compiled binary C++ logic), 2. huffman_coding.js (The JavaScript glue code that loads the .wasm file)

# Compiling both cpp files together into one WebAssembly module
# -lembind: Links the Embind library (allows binding C++ classes/functions to JS)
# -s MODULARIZE=1: Wraps the generated JS in a function, preventing global scope pollution
# -s EXPORT_NAME="HuffmanCodingModule": Defines the global variable name React will use to load the module

emcc huffman_coding_algorithm.cpp wasm_bindings.cpp -o huffman_coding.js -lembind -s MODULARIZE=1 -s EXPORT_NAME="HuffmanCodingModule"

echo "Compilation complete."