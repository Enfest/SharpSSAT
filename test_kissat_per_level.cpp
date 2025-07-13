#include "instance.h"
#include "kissat_integration.h"
#include <iostream>
#include <vector>

int main() {
    std::cout << "=== SharpSSAT Kissat Per-Level Integration Test ===" << std::endl;
    
    // Create an instance
    Instance instance;
    
    // Load a test SDIMACS file
    if (!instance.createfromFile("test_kissat.sdimacs")) {
        std::cerr << "Failed to load test file" << std::endl;
        return 1;
    }
    
    std::cout << "Loaded instance with " << instance.num_variables() << " variables" << std::endl;
    std::cout << "Number of quantifier levels: " << instance.statistics_.num_qlev << std::endl;
    
    // Initialize Kissat solvers for each quantifier level
    if (!instance.initLevelKissatSolvers()) {
        std::cerr << "Failed to initialize level Kissat solvers" << std::endl;
        return 1;
    }
    
    std::cout << "Initialized " << instance.level_kissat_solvers_.size() << " Kissat solvers" << std::endl;
    
    // Test solving for each level
    for (int level = 0; level <= instance.statistics_.num_qlev; ++level) {
        std::cout << "\n--- Testing Level " << level << " ---" << std::endl;
        
        // Check if formula is suitable for Kissat at this level
        if (!instance.isFormulaSuitableForLevelKissat(level)) {
            std::cout << "Level " << level << " is not suitable for Kissat (contains universal variables)" << std::endl;
            continue;
        }
        
        // Get variables at this level
        std::vector<VariableIndex> level_vars = instance.getVariablesAtLevel(level);
        std::cout << "Variables at level " << level << ": ";
        for (auto v : level_vars) {
            std::cout << v << " ";
        }
        std::cout << std::endl;
        
        // Build CNF for this level
        std::vector<std::vector<int>> level_cnf = instance.buildCNFForLevel(level);
        std::cout << "CNF clauses for level " << level << ": " << level_cnf.size() << std::endl;
        
        if (level_cnf.empty()) {
            std::cout << "No clauses at level " << level << std::endl;
            continue;
        }
        
        // Print some clauses for debugging
        for (size_t i = 0; i < std::min(level_cnf.size(), size_t(3)); ++i) {
            std::cout << "Clause " << i << ": ";
            for (int lit : level_cnf[i]) {
                std::cout << lit << " ";
            }
            std::cout << "0" << std::endl;
        }
        
        // Solve with level-specific Kissat
        int result = instance.solveWithLevelKissat(level, level_cnf);
        
        std::cout << "Kissat result for level " << level << ": ";
        switch (result) {
            case 10:
                std::cout << "SAT" << std::endl;
                break;
            case 20:
                std::cout << "UNSAT" << std::endl;
                break;
            default:
                std::cout << "UNKNOWN/ERROR (" << result << ")" << std::endl;
                break;
        }
        
        // Get solution if SAT
        if (result == 10) {
            KissatIntegration* level_solver = instance.getKissatSolverForLevel(level);
            if (level_solver) {
                std::vector<int> solution = level_solver->get_solution();
                std::cout << "Solution for level " << level << ": ";
                for (size_t i = 0; i < std::min(solution.size(), size_t(10)); ++i) {
                    std::cout << solution[i] << " ";
                }
                if (solution.size() > 10) {
                    std::cout << "...";
                }
                std::cout << std::endl;
            }
        }
    }
    
    // Cleanup
    instance.cleanupLevelKissatSolvers();
    
    std::cout << "\n=== Test completed ===" << std::endl;
    return 0;
} 