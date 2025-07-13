#!/bin/bash

# Test script for SharpSSAT-Kissat integration

echo "=== SharpSSAT-Kissat Integration Test ==="
echo

# Check if SharpSSAT-Kissat executable exists
if [ ! -f "./SharpSSAT-Kissat" ]; then
    echo "Error: SharpSSAT-Kissat executable not found!"
    echo "Please build it first using: make -f makefile_kissat"
    exit 1
fi

# Check if Kissat library exists
if [ ! -f "../kissat/libkissat.a" ]; then
    echo "Error: Kissat library not found!"
    echo "Please build Kissat first:"
    echo "  cd ../kissat"
    echo "  ./configure && make"
    echo "  cd ../SharpSSAT"
    exit 1
fi

echo "✓ SharpSSAT-Kissat executable found"
echo "✓ Kissat library found"
echo

# Create a simple test SDIMACS file
echo "Creating test SDIMACS file..."
cat > test_kissat.sdimacs << 'EOF'
c Test SDIMACS file for SharpSSAT-Kissat integration
c Variables: 3 existential, 2 random
c Quantifier prefix: E R E R E
p sdimacs 5 3
e 1 0
r 2 0.5 0
e 3 0
r 4 0.3 0
e 5 0
c Clauses
1 2 0
-2 3 0
-3 4 0
-4 5 0
-1 -5 0
EOF

echo "✓ Test SDIMACS file created: test_kissat.sdimacs"
echo

# Test 1: Basic SSAT solving without Kissat
echo "Test 1: Basic SSAT solving (without Kissat)"
./SharpSSAT-Kissat -s -q test_kissat.sdimacs
echo

# Test 2: SSAT solving with Kissat
echo "Test 2: SSAT solving with Kissat"
./SharpSSAT-Kissat --kissat -s -q test_kissat.sdimacs
echo

# Test 3: SSAT solving with Kissat and limits
echo "Test 3: SSAT solving with Kissat and limits"
./SharpSSAT-Kissat --kissat --kissat-conflicts 1000 --kissat-decisions 500 -s -q test_kissat.sdimacs
echo

# Test 4: Strategy generation with Kissat
echo "Test 4: Strategy generation with Kissat"
./SharpSSAT-Kissat --kissat -s -k -q test_kissat.sdimacs
echo

# Test 5: Certificate generation with Kissat
echo "Test 5: Certificate generation with Kissat"
./SharpSSAT-Kissat --kissat -s -l -q test_kissat.sdimacs
echo

# Test 6: Performance comparison
echo "Test 6: Performance comparison"
echo "Running without Kissat..."
time ./SharpSSAT-Kissat -s -q test_kissat.sdimacs > /dev/null 2>&1
echo "Running with Kissat..."
time ./SharpSSAT-Kissat --kissat -s -q test_kissat.sdimacs > /dev/null 2>&1
echo

# Cleanup
echo "Cleaning up test files..."
rm -f test_kissat.sdimacs test_kissat.blif test_kissat_up.nnf test_kissat_low.nnf test_kissat.prob
echo "✓ Test files cleaned up"
echo

echo "=== Integration test completed ==="
echo
echo "If all tests passed, the SharpSSAT-Kissat integration is working correctly!"
echo
echo "Usage examples:"
echo "  ./SharpSSAT-Kissat --kissat -s input.sdimacs"
echo "  ./SharpSSAT-Kissat --kissat -s -k input.sdimacs"
echo "  ./SharpSSAT-Kissat --kissat --kissat-time 300 -s input.sdimacs" 