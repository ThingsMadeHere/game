#!/bin/bash

echo "Fixing CMake cache issue..."

# Clean build directory
echo "Removing old build directory..."
rm -rf build

# Create new build directory
echo "Creating fresh build directory..."
mkdir build
cd build

# Run cmake with correct source directory
echo "Running cmake..."
cmake ..

# Build the project
echo "Building project..."
make -j4

echo "CMake fix complete!"
