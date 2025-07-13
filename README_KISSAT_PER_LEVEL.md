# SharpSSAT with Kissat Per-Level Integration

This document explains how to use the enhanced SharpSSAT solver with Kissat integration for each quantifier level.

## Overview

The SharpSSAT-Kissat integration now supports **per-quantifier-level solving**, where each quantifier level gets its own dedicated Kissat solver instance. This provides several advantages:

1. **Level-specific optimization**: Each Kissat solver can be configured specifically for the variables and clauses at that level
2. **Better memory management**: Solvers are created and destroyed as needed
3. **Parallel potential**: Future versions could solve multiple levels in parallel
4. **Incremental solving**: Solvers can be reused within the same level

## Architecture

### Quantifier Level Structure

SharpSSAT uses a **prefix-based approach** for SSAT problems:

```
Prefix: [QLevel0, QLevel1, QLevel2, ...]
QLevel: (QuantifierType, [Variables])
```

**Example SDIMACS:**
```
p sdimacs 5 4
e 1 2 0        # Level 0: Existential variables 1,2
r 0.5 3 0      # Level 1: Random variable 3 (prob=0.5)
e 4 0          # Level 2: Existential variable 4
r 0.3 5 0      # Level 3: Random variable 5 (prob=0.3)
1 2 0          # Clauses follow
-2 3 0
-3 4 0
-4 5 0
```

### Integration Levels

1. **Level 0**: Innermost quantifier level (processed first)
2. **Level 1**: Second innermost level
3. **...**: Progressive levels
4. **Level N**: Outermost quantifier level

## Implementation Details

### Core Components

#### 1. Instance Class Extensions

The `Instance` class has been extended with Kissat solver management:

```cpp
class Instance {
private:
    // Kissat solvers for each quantifier level
    vector<KissatIntegration*> level_kissat_solvers_;
    
public:
    // Kissat solver management
    bool initLevelKissatSolvers();
    KissatIntegration* getKissatSolverForLevel(int level);
    int solveWithLevelKissat(int level, const std::vector<std::vector<int>>& clauses);
    void cleanupLevelKissatSolvers();
    
    // CNF conversion utilities
    std::vector<std::vector<int>> buildCNFForLevel(int level);
    std::vector<int> convertLiteralsToCNF(const std::vector<LiteralID>& literals);
    std::vector<LiteralID> convertCNFToLiterals(const std::vector<int>& cnf_literals);
    
    // Solution management
    void applyKissatSolution(const std::vector<int>& solution);
    
    // Utility functions
    bool isFormulaSuitableForLevelKissat(int level);
    std::vector<VariableIndex> getVariablesAtLevel(int level);
};
```

#### 2. Level-Specific CNF Building

The `buildCNFForLevel(int level)` method constructs CNF formulas specific to each quantifier level:

```cpp
std::vector<std::vector<int>> Instance::buildCNFForLevel(int level) {
    std::vector<std::vector<int>> cnf_clauses;
    
    // Get variables at this level
    std::vector<VariableIndex> level_vars = getVariablesAtLevel(level);
    
    // Build CNF from unit clauses
    for (const auto& lit : unit_clauses_) {
        if (qlev(lit) == level && isActive(lit)) {
            std::vector<int> clause = {lit.toInt()};
            cnf_clauses.push_back(clause);
        }
    }
    
    // Build CNF from binary clauses
    // Build CNF from long clauses
    // ... (implementation details)
    
    return cnf_clauses;
}
```

#### 3. Solver Management

Each quantifier level gets its own Kissat solver instance:

```cpp
bool Instance::initLevelKissatSolvers() {
    cleanupLevelKissatSolvers();
    
    for (int level = 0; level <= statistics_.num_qlev; ++level) {
        KissatIntegration* level_solver = new KissatIntegration();
        if (!level_solver->init()) {
            return false;
        }
        level_kissat_solvers_.push_back(level_solver);
    }
    
    return true;
}
```

## Usage Examples

### Basic Usage

```cpp
#include "instance.h"
#include "kissat_integration.h"

int main() {
    Instance instance;
    
    // Load SDIMACS file
    if (!instance.createfromFile("input.sdimacs")) {
        return 1;
    }
    
    // Initialize Kissat solvers for each level
    if (!instance.initLevelKissatSolvers()) {
        return 1;
    }
    
    // Solve each level
    for (int level = 0; level <= instance.statistics_.num_qlev; ++level) {
        if (!instance.isFormulaSuitableForLevelKissat(level)) {
            continue; // Skip levels with universal variables
        }
        
        std::vector<std::vector<int>> level_cnf = instance.buildCNFForLevel(level);
        int result = instance.solveWithLevelKissat(level, level_cnf);
        
        if (result == 10) { // SAT
            KissatIntegration* solver = instance.getKissatSolverForLevel(level);
            std::vector<int> solution = solver->get_solution();
            instance.applyKissatSolution(solution);
        }
    }
    
    // Cleanup
    instance.cleanupLevelKissatSolvers();
    
    return 0;
}
```

### Advanced Usage with Configuration

```cpp
#include "kissat_solver.h"

int main() {
    KissatSolver solver;
    
    // Configure per-level solving
    solver.config().use_kissat = true;
    solver.config().kissat_per_level = true;
    solver.config().kissat_conflict_limit = 1000;
    solver.config().kissat_time_limit = 60;
    
    // Solve with per-level Kissat
    bool result = solver.solve_ind("input.sdimacs");
    
    return result ? 0 : 1;
}
```

## Building and Testing

### Build Instructions

1. **Build Kissat library**:
   ```bash
   cd kissat
   ./configure && make
   cd ..
   ```

2. **Build SharpSSAT with Kissat integration**:
   ```bash
   cd SharpSSAT
   make -f makefile_kissat
   ```

3. **Build test program**:
   ```bash
   make -f makefile_kissat test_kissat_per_level
   ```

### Running Tests

```bash
# Run the test program
./test_kissat_per_level

# Run with a specific SDIMACS file
./test_kissat_per_level < input.sdimacs
```

### Test Output Example

```
=== SharpSSAT Kissat Per-Level Integration Test ===
Loaded instance with 5 variables
Number of quantifier levels: 3
Initialized 4 Kissat solvers

--- Testing Level 0 ---
Variables at level 0: 1 2
CNF clauses for level 0: 1
Clause 0: 1 2 0
Kissat result for level 0: SAT
Solution for level 0: 1 2 ...

--- Testing Level 1 ---
Variables at level 1: 3
CNF clauses for level 1: 1
Clause 0: -2 3 0
Kissat result for level 1: SAT
Solution for level 1: 3 ...

--- Testing Level 2 ---
Variables at level 2: 4
CNF clauses for level 2: 1
Clause 0: -3 4 0
Kissat result for level 2: SAT
Solution for level 2: 4 ...

--- Testing Level 3 ---
Variables at level 3: 5
CNF clauses for level 3: 1
Clause 0: -4 5 0
Kissat result for level 3: SAT
Solution for level 3: 5 ...

=== Test completed ===
```

## Configuration Options

### KissatSolverConfiguration

```cpp
struct KissatSolverConfiguration {
    bool use_kissat = false;           // Enable Kissat integration
    bool kissat_per_level = false;     // Use per-level solving
    bool kissat_incremental = false;   // Use incremental solving
    unsigned kissat_conflict_limit = 0; // Conflict limit
    unsigned kissat_decision_limit = 0; // Decision limit
    int kissat_time_limit = 0;         // Time limit (seconds)
};
```

### Command Line Options

```bash
# Enable per-level Kissat solving
./SharpSSAT-Kissat --kissat --kissat-per-level input.sdimacs

# With limits
./SharpSSAT-Kissat --kissat --kissat-per-level \
    --kissat-conflicts 1000 \
    --kissat-decisions 500 \
    --kissat-time 30 \
    input.sdimacs
```

## Performance Considerations

### Advantages

1. **Level-specific optimization**: Each solver can be tuned for its specific variables
2. **Memory efficiency**: Solvers are created only when needed
3. **Better scalability**: Can handle larger problems by solving level by level
4. **Incremental potential**: Solvers can be reused within the same level

### Limitations

1. **Universal variables**: Kissat is not suitable for universal quantifiers
2. **Overhead**: Creating multiple solvers has some overhead
3. **Memory usage**: Multiple solvers use more memory than a single solver

### Best Practices

1. **Use for existential/random levels**: Kissat works best for existential and random variables
2. **Set appropriate limits**: Use conflict and time limits to prevent long-running solves
3. **Monitor memory usage**: Multiple solvers can use significant memory
4. **Profile performance**: Test with your specific problem instances

## Troubleshooting

### Common Issues

1. **Kissat initialization failed**:
   - Check if Kissat library is properly built
   - Verify library path in makefile

2. **Memory issues**:
   - Reduce number of simultaneous solvers
   - Set lower conflict/decision limits

3. **Performance degradation**:
   - Disable per-level solving for simple problems
   - Use single Kissat solver instead

### Debugging

Enable verbose output to see solver behavior:

```cpp
solver.config().verbose = true;
```

## Future Enhancements

1. **Parallel solving**: Solve multiple levels simultaneously
2. **Adaptive solver selection**: Choose between Kissat and SharpSAT based on problem characteristics
3. **Hybrid approaches**: Use different solvers for different levels
4. **Advanced heuristics**: Problem-specific heuristics for solver configuration

## References

- [SharpSSAT Paper](https://ojs.aaai.org/index.php/AAAI/article/view/25509)
- [Kissat SAT Solver](https://github.com/arminbiere/kissat)
- [SharpSAT](https://github.com/marcthurley/sharpSAT) 