#!/bin/bash

# Build script for SharpSSAT with CaDiCaL integration

set -e  # Exit on any error

echo "=== Building SharpSSAT with CaDiCaL Integration ==="

# Check if we're in the right directory
if [ ! -f "makefile_cadical" ]; then
    echo "Error: makefile_cadical not found. Please run this script from the SharpSSAT directory."
    exit 1
fi

# Check if CaDiCaL directory exists
if [ ! -d "../cadical" ]; then
    echo "Error: CaDiCaL directory not found at ../cadical"
    echo "Please make sure CaDiCaL is cloned in the parent directory."
    exit 1
fi

echo "1. Building CaDiCaL library..."
cd ../cadical
if [ ! -f "configure" ]; then
    echo "Error: CaDiCaL configure script not found. Please run 'git submodule update --init' or clone CaDiCaL properly."
    exit 1
fi

./configure
make -j$(nproc)
cd ../SharpSSAT

echo "2. Building SharpSSAT with CaDiCaL integration..."
make -f makefile_cadical -j$(nproc)

echo "3. Build completed successfully!"
echo "   Executable: ./SharpSSAT-CaDiCaL"
echo ""
echo "Usage examples:"
echo "  ./SharpSSAT-CaDiCaL input.sdimacs"
echo "  ./SharpSSAT-CaDiCaL -s input.sdimacs"
echo "  ./SharpSSAT-CaDiCaL -s -k input.sdimacs  # with strategy generation"
echo ""
echo "Note: This version uses CaDiCaL for incremental SAT solving instead of Kissat." 