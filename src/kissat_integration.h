#ifndef KISSAT_INTEGRATION_H
#define KISSAT_INTEGRATION_H

#include <string>
#include <vector>

// Forward declaration of Kissat solver
typedef struct kissat kissat;

// Wrapper class for Kissat integration
class KissatWrapper {
public:
    KissatWrapper();
    ~KissatWrapper();
    
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
    void add_assumptions(std::vector<int> lit);
    
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
    bool is_initialized() const { return solver_ != nullptr; }
    
    // Reset solver for reuse
    void reset();

    // add clause
    void add_clause(const std::vector<int>& clause);

    // Reserve variables for the solver
    void reserve(int max_var);

private:
    kissat* solver_;
    bool initialized_;
    
    // Disable copy constructor and assignment
    KissatWrapper(const KissatWrapper&) = delete;
    KissatWrapper& operator=(const KissatWrapper&) = delete;
};

#endif // KISSAT_INTEGRATION_H 