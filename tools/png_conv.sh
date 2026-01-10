#!/bin/bash

# Compile the PNG to C converter
echo "Compiling PNG to C converter..."
gcc -o ./tools/png_to_c ./tools/png_to_.c -lpng

if [ $? -eq 0 ]; then
    echo "Successfully compiled png_to_c"
    echo "Usage: ./png_to_c <input.png> <output.h> <varname>"
else
    echo "Failed to compile. Make sure libpng is installed:"
    echo "  Ubuntu/Debian: sudo apt-get install libpng-dev"
    echo "  Fedora/RHEL: sudo dnf install libpng-devel"
    echo "  macOS: brew install libpng"
fi
