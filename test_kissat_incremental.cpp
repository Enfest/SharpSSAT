#include "instance.h"
#include "kissat_integration.h"
#include <iostream>
#include <vector>

int main() {
    std::cout << "=== SharpSSAT Kissat Incremental Solving Test ===" << std::endl;
    
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
    
    // Test incremental solving for each level
    for (int level = 0; level <= instance.statistics_.num_qlev; ++level) {
        std::cout << "\n--- Testing Incremental Solving for Level " << level << " ---" << std::endl;
        
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
        
        // Test 1: Add clauses incrementally
        std::cout << "\nTest 1: Adding clauses incrementally" << std::endl;
        
        // Add unit clauses first
        for (const auto& lit : instance.unit_clauses_) {
            if (instance.qlev(lit) == level && instance.isActive(lit)) {
                std::cout << "Adding unit clause: " << lit.toInt() << std::endl;
                instance.addUnitClauseToLevelKissat(level, lit);
            }
        }
        
        // Add binary clauses
        for (VariableIndex v : level_vars) {
            LiteralID pos_lit(v, false);
            if (instance.isActive(pos_lit)) {
                for (auto it = instance.literal(pos_lit).binary_links_.begin(); 
                     *it != SENTINEL_LIT; ++it) {
                    if (instance.qlev(*it) == level && instance.isActive(*it)) {
                        std::cout << "Adding binary clause: " << pos_lit.toInt() << " " << it->toInt() << std::endl;
                        instance.addBinaryClauseToLevelKissat(level, pos_lit, *it);
                    }
                }
            }
        }
        
        // Add long clauses
        for (auto it_lit = instance.literal_pool_.begin(); it_lit != instance.literal_pool_.end(); ++it_lit) {
            if (*it_lit == SENTINEL_LIT) {
                if (it_lit + 1 == instance.literal_pool_.end()) break;
                it_lit += ClauseHeader::overheadInLits();
                
                // Check if this clause has variables from the current level
                bool has_level_vars = false;
                std::vector<LiteralID> clause;
                
                for (auto clause_it = it_lit; *clause_it != SENTINEL_LIT; ++clause_it) {
                    if (instance.qlev(*clause_it) == level && instance.isActive(*clause_it)) {
                        has_level_vars = true;
                        clause.push_back(*clause_it);
                    }
                }
                
                if (has_level_vars && !clause.empty()) {
                    std::cout << "Adding long clause: ";
                    for (const auto& lit : clause) {
                        std::cout << lit.toInt() << " ";
                    }
                    std::cout << std::endl;
                    instance.addClauseToLevelKissat(level, clause);
                }
            }
        }
        
        // Test 2: Solve with assumptions
        std::cout << "\nTest 2: Solving with assumptions" << std::endl;
        
        // Add some assumptions
        for (VariableIndex v : level_vars) {
            if (v <= 3) { // Only add assumptions for first few variables
                LiteralID pos_lit(v, false);
                std::cout << "Adding assumption: " << pos_lit.toInt() << std::endl;
                instance.addAssumptionToLevelKissat(level, pos_lit);
            }
        }
        
        // Solve with current assumptions
        int result = instance.solveLevelKissatWithAssumptions(level);
        
        std::cout << "Kissat result with assumptions for level " << level << ": ";
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
        
        // Clear assumptions and try different ones
        std::cout << "\nTest 3: Solving with different assumptions" << std::endl;
        instance.clearAssumptionsForLevelKissat(level);
        
        // Add negative assumptions
        for (VariableIndex v : level_vars) {
            if (v <= 2) { // Only add assumptions for first few variables
                LiteralID neg_lit(v, true);
                std::cout << "Adding negative assumption: " << neg_lit.toInt() << std::endl;
                instance.addAssumptionToLevelKissat(level, neg_lit);
            }
        }
        
        // Solve again
        result = instance.solveLevelKissatWithAssumptions(level);
        
        std::cout << "Kissat result with negative assumptions for level " << level << ": ";
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
        
        // Test 4: Direct solver access for advanced usage
        std::cout << "\nTest 4: Direct solver access" << std::endl;
        KissatIntegration* level_solver = instance.getKissatSolverForLevel(level);
        if (level_solver) {
            KissatWrapper* wrapper = level_solver->get_wrapper();
            if (wrapper) {
                std::cout << "Solver is initialized: " << (wrapper->is_initialized() ? "Yes" : "No") << std::endl;
                
                // Print statistics
                std::cout << "Solver statistics:" << std::endl;
                wrapper->print_statistics();
            }
        }
    }
    
    // Cleanup
    instance.cleanupLevelKissatSolvers();
    
    std::cout << "\n=== Incremental Test completed ===" << std::endl;
    return 0;
} 