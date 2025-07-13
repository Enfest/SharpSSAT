#include "instance.h"
#include "kissat_integration.h"
#include <iostream>
#include <vector>

// Example: How to use incremental solving with Kissat for each quantifier level
int main() {
    std::cout << "=== SharpSSAT Kissat Incremental Solving Example ===" << std::endl;
    
    // Create an instance
    Instance instance;
    
    // Load a test SDIMACS file
    if (!instance.createfromFile("test_kissat.sdimacs")) {
        std::cerr << "Failed to load test file" << std::endl;
        return 1;
    }
    
    // Initialize Kissat solvers for each quantifier level
    if (!instance.initLevelKissatSolvers()) {
        std::cerr << "Failed to initialize level Kissat solvers" << std::endl;
        return 1;
    }
    
    std::cout << "Initialized " << instance.level_kissat_solvers_.size() << " Kissat solvers" << std::endl;
    
    // Example 1: Incremental clause addition
    std::cout << "\n=== Example 1: Incremental Clause Addition ===" << std::endl;
    
    for (int level = 0; level <= instance.statistics_.num_qlev; ++level) {
        if (!instance.isFormulaSuitableForLevelKissat(level)) {
            continue;
        }
        
        std::cout << "\nLevel " << level << ":" << std::endl;
        
        // Step 1: Add unit clauses
        for (const auto& lit : instance.unit_clauses_) {
            if (instance.qlev(lit) == level && instance.isActive(lit)) {
                std::cout << "  Adding unit clause: " << lit.toInt() << std::endl;
                instance.addUnitClauseToLevelKissat(level, lit);
            }
        }
        
        // Step 2: Add binary clauses
        std::vector<VariableIndex> level_vars = instance.getVariablesAtLevel(level);
        for (VariableIndex v : level_vars) {
            LiteralID pos_lit(v, false);
            if (instance.isActive(pos_lit)) {
                for (auto it = instance.literal(pos_lit).binary_links_.begin(); 
                     *it != SENTINEL_LIT; ++it) {
                    if (instance.qlev(*it) == level && instance.isActive(*it)) {
                        std::cout << "  Adding binary clause: " << pos_lit.toInt() << " " << it->toInt() << std::endl;
                        instance.addBinaryClauseToLevelKissat(level, pos_lit, *it);
                    }
                }
            }
        }
        
        // Step 3: Add long clauses
        for (auto it_lit = instance.literal_pool_.begin(); it_lit != instance.literal_pool_.end(); ++it_lit) {
            if (*it_lit == SENTINEL_LIT) {
                if (it_lit + 1 == instance.literal_pool_.end()) break;
                it_lit += ClauseHeader::overheadInLits();
                
                std::vector<LiteralID> clause;
                bool has_level_vars = false;
                
                for (auto clause_it = it_lit; *clause_it != SENTINEL_LIT; ++clause_it) {
                    if (instance.qlev(*clause_it) == level && instance.isActive(*clause_it)) {
                        has_level_vars = true;
                        clause.push_back(*clause_it);
                    }
                }
                
                if (has_level_vars && !clause.empty()) {
                    std::cout << "  Adding long clause: ";
                    for (const auto& lit : clause) {
                        std::cout << lit.toInt() << " ";
                    }
                    std::cout << std::endl;
                    instance.addClauseToLevelKissat(level, clause);
                }
            }
        }
    }
    
    // Example 2: Assumption-based solving
    std::cout << "\n=== Example 2: Assumption-Based Solving ===" << std::endl;
    
    for (int level = 0; level <= instance.statistics_.num_qlev; ++level) {
        if (!instance.isFormulaSuitableForLevelKissat(level)) {
            continue;
        }
        
        std::cout << "\nLevel " << level << ":" << std::endl;
        
        std::vector<VariableIndex> level_vars = instance.getVariablesAtLevel(level);
        
        // Test different assumption combinations
        std::vector<std::vector<int>> assumption_sets = {
            {1, 2},      // Positive assumptions
            {-1, -2},    // Negative assumptions
            {1, -2},     // Mixed assumptions
            {}           // No assumptions
        };
        
        for (size_t i = 0; i < assumption_sets.size(); ++i) {
            std::cout << "  Test " << (i+1) << " - Assumptions: ";
            if (assumption_sets[i].empty()) {
                std::cout << "none";
            } else {
                for (int assump : assumption_sets[i]) {
                    std::cout << assump << " ";
                }
            }
            std::cout << std::endl;
            
            // Clear previous assumptions
            instance.clearAssumptionsForLevelKissat(level);
            
            // Add new assumptions
            for (int assump : assumption_sets[i]) {
                LiteralID lit(assump);
                if (instance.qlev(lit) == level) {
                    instance.addAssumptionToLevelKissat(level, lit);
                }
            }
            
            // Solve with assumptions
            int result = instance.solveLevelKissatWithAssumptions(level);
            
            std::cout << "    Result: ";
            switch (result) {
                case 10:
                    std::cout << "SAT";
                    break;
                case 20:
                    std::cout << "UNSAT";
                    break;
                default:
                    std::cout << "UNKNOWN (" << result << ")";
                    break;
            }
            std::cout << std::endl;
        }
    }
    
    // Example 3: Direct solver access for advanced usage
    std::cout << "\n=== Example 3: Direct Solver Access ===" << std::endl;
    
    for (int level = 0; level <= instance.statistics_.num_qlev; ++level) {
        if (!instance.isFormulaSuitableForLevelKissat(level)) {
            continue;
        }
        
        std::cout << "\nLevel " << level << ":" << std::endl;
        
        KissatIntegration* level_solver = instance.getKissatSolverForLevel(level);
        if (level_solver) {
            // Get direct access to the wrapper
            KissatWrapper* wrapper = level_solver->get_wrapper();
            if (wrapper) {
                std::cout << "  Solver initialized: " << (wrapper->is_initialized() ? "Yes" : "No") << std::endl;
                
                // Set solver limits
                wrapper->set_conflict_limit(1000);
                wrapper->set_decision_limit(500);
                
                // Print statistics
                std::cout << "  Solver statistics:" << std::endl;
                wrapper->print_statistics();
            }
        }
    }
    
    // Example 4: Building CNF incrementally for a specific level
    std::cout << "\n=== Example 4: Building CNF Incrementally ===" << std::endl;
    
    int target_level = 1; // Focus on level 1
    if (target_level <= instance.statistics_.num_qlev && 
        instance.isFormulaSuitableForLevelKissat(target_level)) {
        
        std::cout << "Building CNF for level " << target_level << " incrementally:" << std::endl;
        
        // Get the solver for this level
        KissatIntegration* solver = instance.getKissatSolverForLevel(target_level);
        if (solver) {
            // Reset the solver
            solver->reset_solver();
            
            // Add clauses one by one
            std::vector<VariableIndex> level_vars = instance.getVariablesAtLevel(target_level);
            
            // Add unit clauses
            for (const auto& lit : instance.unit_clauses_) {
                if (instance.qlev(lit) == target_level && instance.isActive(lit)) {
                    std::cout << "  Adding unit: " << lit.toInt() << std::endl;
                    solver->add_unit_clause(lit.toInt());
                }
            }
            
            // Add binary clauses
            for (VariableIndex v : level_vars) {
                LiteralID pos_lit(v, false);
                if (instance.isActive(pos_lit)) {
                    for (auto it = instance.literal(pos_lit).binary_links_.begin(); 
                         *it != SENTINEL_LIT; ++it) {
                        if (instance.qlev(*it) == target_level && instance.isActive(*it)) {
                            std::cout << "  Adding binary: " << pos_lit.toInt() << " " << it->toInt() << std::endl;
                            solver->add_binary_clause(pos_lit.toInt(), it->toInt());
                        }
                    }
                }
            }
            
            // Solve
            int result = solver->solve_with_current_assumptions();
            std::cout << "  Result: " << (result == 10 ? "SAT" : result == 20 ? "UNSAT" : "UNKNOWN") << std::endl;
        }
    }
    
    // Cleanup
    instance.cleanupLevelKissatSolvers();
    
    std::cout << "\n=== Example completed ===" << std::endl;
    return 0;
} 