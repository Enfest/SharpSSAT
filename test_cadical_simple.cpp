#include "src/cadical_integration.h"
#include <iostream>
#include <vector>

int main() {
    std::cout << "=== Simple CaDiCaL Test ===" << std::endl;
    
    // Test 1: Basic solver creation and destruction
    {
        std::cout << "\n--- Test 1: Basic solver ---" << std::endl;
        CaDiCaLWrapper* wrapper = new CaDiCaLWrapper();
        if (wrapper->init()) {
            std::cout << "Solver initialized successfully" << std::endl;
        } else {
            std::cout << "Failed to initialize solver" << std::endl;
            delete wrapper;
            return 1;
        }
        delete wrapper;
        std::cout << "Solver deleted successfully" << std::endl;
    }
    
    // Test 2: Solve a simple SAT formula
    {
        std::cout << "\n--- Test 2: Simple SAT formula ---" << std::endl;
        CaDiCaLWrapper wrapper;
        if (!wrapper.init()) {
            std::cout << "Failed to initialize solver" << std::endl;
            return 1;
        }
        
        // Add clauses: (x1) AND (x1 OR x2) AND (NOT x1 OR x2)
        wrapper.add_clause({1});           // x1
        wrapper.add_clause({1, 2});        // x1 OR x2
        wrapper.add_clause({-1, 2});       // NOT x1 OR x2
        
        int result = wrapper.solve();
        std::cout << "Solve result: " << result << std::endl;
        
        if (result == 10) { // SAT
            std::cout << "SAT - Formula is satisfiable" << std::endl;
            int val1 = wrapper.get_value(1);
            int val2 = wrapper.get_value(2);
            std::cout << "x1 = " << (val1 > 0 ? "true" : "false") << std::endl;
            std::cout << "x2 = " << (val2 > 0 ? "true" : "false") << std::endl;
        } else if (result == 20) {
            std::cout << "UNSAT - Formula is unsatisfiable" << std::endl;
        } else {
            std::cout << "UNKNOWN - Timeout or error" << std::endl;
        }
    }
    
    // Test 3: Solve an UNSAT formula
    {
        std::cout << "\n--- Test 3: UNSAT formula ---" << std::endl;
        CaDiCaLWrapper wrapper;
        if (!wrapper.init()) {
            std::cout << "Failed to initialize solver" << std::endl;
            return 1;
        }
        
        // Add clauses: (x1) AND (NOT x1)
        wrapper.add_clause({1});           // x1
        wrapper.add_clause({-1});          // NOT x1
        
        int result = wrapper.solve();
        std::cout << "Solve result: " << result << std::endl;
        
        if (result == 10) {
            std::cout << "SAT - Formula is satisfiable" << std::endl;
        } else if (result == 20) {
            std::cout << "UNSAT - Formula is unsatisfiable" << std::endl;
        } else {
            std::cout << "UNKNOWN - Timeout or error" << std::endl;
        }
    }
    
    // Test 4: Test reset functionality
    {
        std::cout << "\n--- Test 4: Reset functionality ---" << std::endl;
        CaDiCaLWrapper wrapper;
        if (!wrapper.init()) {
            std::cout << "Failed to initialize solver" << std::endl;
            return 1;
        }
        
        // Add a clause and solve
        wrapper.add_clause({1});
        int result1 = wrapper.solve();
        std::cout << "First solve result: " << result1 << std::endl;
        
        // Reset and solve again
        wrapper.reset();
        wrapper.add_clause({-1});
        int result2 = wrapper.solve();
        std::cout << "Second solve result: " << result2 << std::endl;
    }
    
    std::cout << "\n=== All tests completed successfully ===" << std::endl;
    return 0;
} 