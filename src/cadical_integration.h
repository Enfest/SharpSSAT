#ifndef CADICAL_INTEGRATION_H
#define CADICAL_INTEGRATION_H

#include <string>
#include <vector>

// Forward declaration of CaDiCaL solver
namespace CaDiCaL {
    class Solver;
}

// Wrapper class for CaDiCaL integration
class CaDiCaLWrapper {
public:
    CaDiCaLWrapper();
    ~CaDiCaLWrapper();
    
    // Initialize the solver
    bool init();
    
    // Add a literal to the current clause
    void add_literal(int lit);
    
    // Finalize the current clause
    void finalize_clause();
    
    // Solve the formula
    int solve();
    
    // Get the value of a literal in the solution
    int get_value(int lit);
    
    // Add assumptions (for incremental solving)
    void add_assumptions(const std::vector<int>& assumptions);
    
    // Clear assumptions
    void clear_assumptions();
    
    // Set termination callback
    void set_terminate_callback(void* state, int (*terminate)(void* state));
    
    // Set time limit (in seconds)
    void set_time_limit(int seconds);
    
    // Set conflict limit
    void set_conflict_limit(unsigned limit);
    
    // Set decision limit
    void set_decision_limit(unsigned limit);
    
    // Get solver statistics
    void print_statistics();
    
    // Check if solver is initialized
    bool is_initialized() const { return solver_ != nullptr && initialized_ && !deleted_; }
    
    // Reset solver for reuse
    void reset();

    // Add clause
    void add_clause(const std::vector<int>& clause);

    // Reserve variables for the solver
    void reserve(int max_var);
    
    // Check if a literal failed (is in the core)
    bool failed(int lit);
    
    // Get the underlying CaDiCaL solver for advanced usage
    CaDiCaL::Solver* get_solver() { return solver_; }

private:
    CaDiCaL::Solver* solver_;
    bool initialized_;
    bool deleted_;  // Flag to prevent double deletion
    
    // Disable copy constructor and assignment
    CaDiCaLWrapper(const CaDiCaLWrapper&) = delete;
    CaDiCaLWrapper& operator=(const CaDiCaLWrapper&) = delete;
};

// High-level integration class for CaDiCaL
class CaDiCaLIntegration {
public:
    CaDiCaLIntegration();
    ~CaDiCaLIntegration();
    
    // Initialize the integration
    bool init();
    
    // Solve a CNF formula
    int solve_cnf(const std::vector<std::vector<int>>& clauses);
    
    // Solve with assumptions
    int solve_with_assumptions(const std::vector<std::vector<int>>& clauses,
                              const std::vector<int>& assumptions);
    
    // Get solution
    std::vector<int> get_solution() const;
    
    // Set limits
    void set_conflict_limit(unsigned limit);
    void set_decision_limit(unsigned limit);
    void set_time_limit(int seconds);
    
    // Get the wrapper for direct access
    CaDiCaLWrapper* get_wrapper() { return wrapper_; }
    
    // Check if initialized
    bool is_initialized() const { return wrapper_ && wrapper_->is_initialized(); }

private:
    CaDiCaLWrapper* wrapper_;
    std::vector<int> last_solution_;
    
    // Disable copy constructor and assignment
    CaDiCaLIntegration(const CaDiCaLIntegration&) = delete;
    CaDiCaLIntegration& operator=(const CaDiCaLIntegration&) = delete;
};

#endif // CADICAL_INTEGRATION_H 