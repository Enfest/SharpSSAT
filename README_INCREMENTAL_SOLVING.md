# SharpSSAT Kissat Incremental Solving Guide

This document explains how to use the **incremental solving capabilities** with Kissat for each quantifier level in SharpSSAT.

## Overview

The incremental solving feature allows you to:

1. **Add clauses one by one** to each Kissat solver
2. **Use assumptions** for solving with temporary constraints
3. **Reuse solvers** across multiple solving calls
4. **Build formulas incrementally** for better performance

## Key Features

### 1. Incremental Clause Addition

You can add clauses to each quantifier level's Kissat solver incrementally:

```cpp
// Add unit clauses
instance.addUnitClauseToLevelKissat(level, LiteralID(1, false));  // x1

// Add binary clauses
instance.addBinaryClauseToLevelKissat(level, LiteralID(1, false), LiteralID(2, true));  // x1 ∨ ¬x2

// Add long clauses
std::vector<LiteralID> clause = {LiteralID(1, false), LiteralID(2, true), LiteralID(3, false)};
instance.addClauseToLevelKissat(level, clause);  // x1 ∨ ¬x2 ∨ x3
```

### 2. Assumption-Based Solving

You can solve with temporary assumptions that don't permanently modify the formula:

```cpp
// Add assumptions
instance.addAssumptionToLevelKissat(level, LiteralID(1, false));  // Assume x1 = true
instance.addAssumptionToLevelKissat(level, LiteralID(2, true));   // Assume x2 = false

// Solve with current assumptions
int result = instance.solveLevelKissatWithAssumptions(level);

// Clear assumptions for next solve
instance.clearAssumptionsForLevelKissat(level);
```

### 3. Direct Solver Access

For advanced usage, you can access the underlying Kissat solver directly:

```cpp
KissatIntegration* solver = instance.getKissatSolverForLevel(level);
KissatWrapper* wrapper = solver->get_wrapper();

// Direct access to Kissat API
wrapper->set_conflict_limit(1000);
wrapper->set_decision_limit(500);
wrapper->print_statistics();
```

## API Reference

### Instance Class Methods

#### Incremental Clause Addition

```cpp
// Add a clause to a specific quantifier level
void addClauseToLevelKissat(int level, const std::vector<LiteralID>& literals);

// Add a unit clause to a specific quantifier level
void addUnitClauseToLevelKissat(int level, LiteralID lit);

// Add a binary clause to a specific quantifier level
void addBinaryClauseToLevelKissat(int level, LiteralID lit1, LiteralID lit2);
```

#### Assumption-Based Solving

```cpp
// Add an assumption to a specific quantifier level
void addAssumptionToLevelKissat(int level, LiteralID lit);

// Clear all assumptions for a specific quantifier level
void clearAssumptionsForLevelKissat(int level);

// Solve with current assumptions for a specific quantifier level
int solveLevelKissatWithAssumptions(int level);
```

#### Solver Management

```cpp
// Get the Kissat solver for a specific level
KissatIntegration* getKissatSolverForLevel(int level);

// Initialize all level solvers
bool initLevelKissatSolvers();

// Clean up all level solvers
void cleanupLevelKissatSolvers();
```

### KissatIntegration Class Methods

#### Incremental Interface

```cpp
// Add a clause (vector of literals)
void add_clause(const std::vector<int>& clause);

// Add a unit clause
void add_unit_clause(int lit);

// Add a binary clause
void add_binary_clause(int lit1, int lit2);
```

#### Assumption Interface

```cpp
// Add an assumption
void add_assumption(int lit);

// Clear all assumptions
void clear_assumptions();

// Solve with current assumptions
int solve_with_current_assumptions();
```

#### Solver Control

```cpp
// Check if solver is initialized
bool is_initialized() const;

// Reset solver for reuse
void reset_solver();

// Get direct access to wrapper
KissatWrapper* get_wrapper();
```

## Usage Examples

### Example 1: Basic Incremental Solving

```cpp
#include "instance.h"
#include "kissat_integration.h"

int main() {
    Instance instance;
    
    // Load formula and initialize solvers
    instance.createfromFile("input.sdimacs");
    instance.initLevelKissatSolvers();
    
    // Add clauses incrementally to level 0
    instance.addUnitClauseToLevelKissat(0, LiteralID(1, false));
    instance.addBinaryClauseToLevelKissat(0, LiteralID(1, false), LiteralID(2, true));
    
    // Solve with assumptions
    instance.addAssumptionToLevelKissat(0, LiteralID(3, false));
    int result = instance.solveLevelKissatWithAssumptions(0);
    
    // Cleanup
    instance.cleanupLevelKissatSolvers();
    return 0;
}
```

### Example 2: Building Formula Incrementally

```cpp
#include "instance.h"
#include "kissat_integration.h"

void buildFormulaIncrementally(Instance& instance, int level) {
    // Get solver for this level
    KissatIntegration* solver = instance.getKissatSolverForLevel(level);
    if (!solver) return;
    
    // Reset solver
    solver->reset_solver();
    
    // Add clauses one by one
    solver->add_unit_clause(1);           // x1
    solver->add_binary_clause(1, -2);     // x1 ∨ ¬x2
    solver->add_binary_clause(-1, 3);     // ¬x1 ∨ x3
    
    // Add a long clause
    std::vector<int> clause = {2, -3, 4};
    solver->add_clause(clause);           // x2 ∨ ¬x3 ∨ x4
    
    // Solve
    int result = solver->solve_with_current_assumptions();
    std::cout << "Result: " << (result == 10 ? "SAT" : "UNSAT") << std::endl;
}
```

### Example 3: Assumption-Based Search

```cpp
#include "instance.h"
#include "kissat_integration.h"

void assumptionBasedSearch(Instance& instance, int level) {
    std::vector<VariableIndex> vars = instance.getVariablesAtLevel(level);
    
    // Try different assumption combinations
    for (VariableIndex v : vars) {
        // Try positive assumption
        instance.addAssumptionToLevelKissat(level, LiteralID(v, false));
        int result_pos = instance.solveLevelKissatWithAssumptions(level);
        
        // Clear and try negative assumption
        instance.clearAssumptionsForLevelKissat(level);
        instance.addAssumptionToLevelKissat(level, LiteralID(v, true));
        int result_neg = instance.solveLevelKissatWithAssumptions(level);
        
        std::cout << "Variable " << v << ": pos=" << result_pos << ", neg=" << result_neg << std::endl;
        
        // Clear for next iteration
        instance.clearAssumptionsForLevelKissat(level);
    }
}
```

### Example 4: Advanced Solver Configuration

```cpp
#include "instance.h"
#include "kissat_integration.h"

void configureSolver(Instance& instance, int level) {
    KissatIntegration* solver = instance.getKissatSolverForLevel(level);
    if (!solver) return;
    
    KissatWrapper* wrapper = solver->get_wrapper();
    if (!wrapper) return;
    
    // Set solver limits
    wrapper->set_conflict_limit(10000);
    wrapper->set_decision_limit(5000);
    
    // Set termination callback (if needed)
    // wrapper->set_terminate_callback(state, callback);
    
    // Print configuration
    std::cout << "Solver configured for level " << level << std::endl;
    wrapper->print_statistics();
}
```

## Best Practices

### 1. Solver Lifecycle Management

```cpp
// Always initialize solvers before use
if (!instance.initLevelKissatSolvers()) {
    // Handle initialization failure
    return;
}

// Use solvers...

// Always cleanup when done
instance.cleanupLevelKissatSolvers();
```

### 2. Assumption Management

```cpp
// Clear assumptions before adding new ones
instance.clearAssumptionsForLevelKissat(level);

// Add assumptions
instance.addAssumptionToLevelKissat(level, lit1);
instance.addAssumptionToLevelKissat(level, lit2);

// Solve
int result = instance.solveLevelKissatWithAssumptions(level);

// Clear for next use
instance.clearAssumptionsForLevelKissat(level);
```

### 3. Error Handling

```cpp
// Check if level is suitable for Kissat
if (!instance.isFormulaSuitableForLevelKissat(level)) {
    // Skip or use alternative solver
    return;
}

// Check if solver exists
KissatIntegration* solver = instance.getKissatSolverForLevel(level);
if (!solver) {
    // Handle missing solver
    return;
}

// Check solver initialization
if (!solver->is_initialized()) {
    // Handle uninitialized solver
    return;
}
```

### 4. Performance Optimization

```cpp
// Reuse solvers when possible
KissatIntegration* solver = instance.getKissatSolverForLevel(level);

// Reset instead of recreating
solver->reset_solver();

// Add clauses in batches for better performance
for (const auto& clause : clause_batch) {
    solver->add_clause(clause);
}

// Solve once after adding all clauses
int result = solver->solve_with_current_assumptions();
```

## Common Patterns

### 1. Incremental Formula Building

```cpp
void buildFormulaIncrementally(Instance& instance, int level) {
    KissatIntegration* solver = instance.getKissatSolverForLevel(level);
    solver->reset_solver();
    
    // Add base clauses
    for (const auto& clause : base_clauses) {
        solver->add_clause(clause);
    }
    
    // Add dynamic clauses
    for (const auto& clause : dynamic_clauses) {
        solver->add_clause(clause);
        // Optionally solve after each addition
        int result = solver->solve_with_current_assumptions();
        if (result == 20) { // UNSAT
            break; // Early termination
        }
    }
}
```

### 2. Assumption-Based Enumeration

```cpp
void enumerateWithAssumptions(Instance& instance, int level) {
    std::vector<VariableIndex> vars = instance.getVariablesAtLevel(level);
    
    // Try all combinations of first few variables
    for (int mask = 0; mask < (1 << std::min(5, (int)vars.size())); ++mask) {
        instance.clearAssumptionsForLevelKissat(level);
        
        for (int i = 0; i < std::min(5, (int)vars.size()); ++i) {
            bool positive = (mask >> i) & 1;
            LiteralID lit(vars[i], !positive);
            instance.addAssumptionToLevelKissat(level, lit);
        }
        
        int result = instance.solveLevelKissatWithAssumptions(level);
        if (result == 10) { // SAT
            // Process satisfying assignment
            processSolution(instance, level);
        }
    }
}
```

### 3. Incremental Learning

```cpp
void incrementalLearning(Instance& instance, int level) {
    KissatIntegration* solver = instance.getKissatSolverForLevel(level);
    
    // Start with base formula
    solver->reset_solver();
    addBaseClauses(solver);
    
    // Learn and add new clauses
    for (int iteration = 0; iteration < max_iterations; ++iteration) {
        int result = solver->solve_with_current_assumptions();
        
        if (result == 10) { // SAT
            // Extract learned clause from solution
            std::vector<int> learned_clause = extractLearnedClause(solver);
            solver->add_clause(learned_clause);
        } else if (result == 20) { // UNSAT
            break; // Formula is unsatisfiable
        }
    }
}
```

## Troubleshooting

### Common Issues

1. **Solver not initialized**:
   ```cpp
   if (!solver->is_initialized()) {
       std::cerr << "Solver not initialized" << std::endl;
       return;
   }
   ```

2. **Invalid level access**:
   ```cpp
   if (level < 0 || level > instance.statistics_.num_qlev) {
       std::cerr << "Invalid level: " << level << std::endl;
       return;
   }
   ```

3. **Memory issues with multiple solvers**:
   ```cpp
   // Set lower limits for multiple solvers
   wrapper->set_conflict_limit(1000);  // Lower limit
   wrapper->set_decision_limit(500);   // Lower limit
   ```

### Debugging Tips

1. **Enable verbose output**:
   ```cpp
   wrapper->print_statistics();
   ```

2. **Check solver state**:
   ```cpp
   std::cout << "Solver initialized: " << solver->is_initialized() << std::endl;
   ```

3. **Monitor assumption count**:
   ```cpp
   // Clear assumptions regularly to prevent accumulation
   instance.clearAssumptionsForLevelKissat(level);
   ```

## Performance Considerations

### Advantages of Incremental Solving

1. **Early termination**: Stop adding clauses when UNSAT is detected
2. **Memory efficiency**: Add clauses only when needed
3. **Assumption flexibility**: Test different scenarios without rebuilding
4. **Solver reuse**: Avoid solver recreation overhead

### When to Use Incremental Solving

- **Dynamic clause generation**: When clauses are generated during solving
- **Assumption-based search**: When testing multiple scenarios
- **Learning scenarios**: When adding learned clauses incrementally
- **Interactive solving**: When user provides constraints incrementally

### Performance Tips

1. **Batch clause additions** when possible
2. **Reuse solvers** instead of recreating
3. **Set appropriate limits** for your problem size
4. **Clear assumptions** regularly to prevent accumulation
5. **Monitor memory usage** with multiple solvers

## References

- [Kissat SAT Solver](https://github.com/arminbiere/kissat)
- [SharpSSAT Documentation](README_KISSAT_PER_LEVEL.md)
- [Incremental SAT Solving](https://en.wikipedia.org/wiki/Boolean_satisfiability_problem#Incremental_solving) 