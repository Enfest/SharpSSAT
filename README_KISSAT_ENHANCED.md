# Enhanced SharpSSAT-Kissat Integration

## Overview

This enhanced integration extends SharpSSAT with Kissat SAT solver capabilities, providing both general SAT solving and **per-quantifier-level SSAT solving**. The integration allows SharpSSAT to leverage Kissat's high-performance SAT solving while maintaining its advanced SSAT capabilities.

## Key Features

### 1. **General Kissat Integration**
- Use Kissat for standard SAT solving within SharpSSAT
- Configurable limits (conflict, decision, time)
- Fallback to original SharpSSAT when Kissat is unsuitable

### 2. **Per-Quantifier-Level Integration** ⭐ **NEW**
- **Separate Kissat instances** for each quantifier level
- **Level-specific solving** for complex SSAT problems
- **Quantifier-aware clause filtering** for optimal performance
- **Progressive solving** from innermost to outermost levels

### 3. **SSAT-Specific Enhancements**
- **Quantifier type awareness** (existential, universal, random)
- **Level-based variable selection** for decision making
- **Probability-aware solving** for random variables
- **Strategy generation** compatibility

## Architecture

### Quantifier Level Structure

SharpSSAT uses a **prefix-based approach** for SSAT problems:

```
Prefix: [QLevel0, QLevel1, QLevel2, ...]
QLevel: (QuantifierType, [Variables])
```

**Example SDIMACS:**
```
p cnf 4 3
e 1 2 0        # Level 0: Existential variables 1,2
r 0.5 3 0      # Level 1: Random variable 3 (prob=0.5)
a 4 0          # Level 2: Universal variable 4
1 2 0          # Clauses follow
-1 3 0
2 -4 0
```

### Integration Levels

1. **Level 0**: Innermost quantifier level (processed first)
2. **Level 1**: Second innermost level
3. **...**: Progressive levels
4. **Level N**: Outermost quantifier level

## Usage

### Basic Kissat Integration

```bash
# Enable Kissat integration
./SharpSSAT-Kissat -kissat input.sdimacs

# With limits
./SharpSSAT-Kissat -kissat -kissat-conflict 1000 -kissat-time 60 input.sdimacs
```

### Per-Level Kissat Integration

```bash
# Enable per-level Kissat solving for SSAT
./SharpSSAT-Kissat -s -kissat -kissat-per-level input.sdimacs

# With comprehensive configuration
./SharpSSAT-Kissat -s -kissat -kissat-per-level \
    -kissat-conflict 500 \
    -kissat-decision 1000 \
    -kissat-time 30 \
    input.sdimacs
```

### Advanced Options

```bash
# SSAT solving with strategy generation and Kissat
./SharpSSAT-Kissat -s -k -kissat -kissat-per-level input.sdimacs

# With universal quantifiers (limited Kissat support)
./SharpSSAT-Kissat -s -u -kissat input.sdimacs

# Certificate generation with Kissat
./SharpSSAT-Kissat -s -l -kissat input.sdimacs
```

## Command Line Options

### Kissat Integration Options

| Option | Description |
|--------|-------------|
| `-kissat` | Enable Kissat integration |
| `-kissat-per-level` | Use separate Kissat instance per quantifier level |
| `-kissat-conflict [n]` | Set Kissat conflict limit |
| `-kissat-decision [n]` | Set Kissat decision limit |
| `-kissat-time [s]` | Set Kissat time limit (seconds) |

### Standard SharpSSAT Options

| Option | Description |
|--------|-------------|
| `-s` | Enable SSAT solving |
| `-k` | Enable strategy generation |
| `-l` | Enable certificate generation |
| `-u` | Enable universal quantifiers |
| `-q` | Quiet mode |
| `-t [s]` | Set time bound |

## Implementation Details

### 1. **Level-Specific Solver Management**

```cpp
class KissatSolver {
private:
    std::map<int, KissatIntegration*> level_kissat_solvers_;
    
    bool initLevelKissatSolvers();
    KissatIntegration* getKissatSolverForLevel(int level);
    int solveWithLevelKissat(int level, const std::vector<std::vector<int>>& clauses);
};
```

### 2. **Quantifier-Aware CNF Building**

```cpp
std::vector<std::vector<int>> buildCNFForLevel(int level) {
    // Get variables at this level
    std::vector<VariableIndex> level_vars = getVariablesAtLevel(level);
    
    // Build clauses involving variables at this level
    // Filter clauses based on variable dependencies
    // Return level-specific CNF
}
```

### 3. **Progressive Solving Strategy**

```cpp
bool useKissatForSSAT() {
    // Solve level by level, innermost first
    for (int level = 0; level <= statistics_.num_qlev; ++level) {
        if (!isFormulaSuitableForLevelKissat(level)) continue;
        
        std::vector<std::vector<int>> level_cnf = buildCNFForLevel(level);
        int result = solveWithLevelKissat(level, level_cnf);
        
        if (result == 10) { // SAT
            applyLevelSolution(level, solution);
            return true;
        } else if (result == 20) { // UNSAT
            return false;
        }
    }
    return useKissatForSAT(); // Fallback
}
```

## Performance Considerations

### 1. **Memory Usage**
- Each quantifier level gets its own Kissat instance
- Memory scales with number of quantifier levels
- Consider using `-kissat-time` to limit resource usage

### 2. **Solving Strategy**
- **Innermost-first**: Process level 0 before level 1
- **Early termination**: Stop at first UNSAT level
- **Fallback mechanism**: Use original SharpSSAT if Kissat fails

### 3. **Clause Filtering**
- Only include clauses relevant to current level
- Avoid redundant clause processing
- Optimize for level-specific variable dependencies

## Limitations and Considerations

### 1. **Universal Quantifiers**
- Limited Kissat support for universal variables
- May fall back to original SharpSSAT for universal-heavy problems
- Use `-u` flag with caution

### 2. **Complex Dependencies**
- Inter-level variable dependencies may limit effectiveness
- Some problems may benefit more from original SharpSSAT
- Monitor performance and adjust strategy accordingly

### 3. **Resource Management**
- Multiple Kissat instances consume more memory
- Set appropriate limits to prevent resource exhaustion
- Consider problem size when choosing integration mode

## Examples

### Example 1: Simple SSAT Problem

**Input (simple.sdimacs):**
```
p cnf 3 2
e 1 2 0
r 0.5 3 0
1 2 0
-1 3 0
```

**Command:**
```bash
./SharpSSAT-Kissat -s -kissat -kissat-per-level simple.sdimacs
```

**Process:**
1. Level 0: Solve with variables 1,2 (existential)
2. Level 1: Solve with variable 3 (random, prob=0.5)
3. Combine results for final probability

### Example 2: Complex Multi-Level Problem

**Input (complex.sdimacs):**
```
p cnf 6 4
e 1 2 0
r 0.3 3 0
e 4 5 0
r 0.7 6 0
1 2 0
-1 3 0
4 -5 0
-3 6 0
```

**Command:**
```bash
./SharpSSAT-Kissat -s -kissat -kissat-per-level \
    -kissat-conflict 1000 \
    -kissat-time 60 \
    complex.sdimacs
```

## Troubleshooting

### Common Issues

1. **"Failed to initialize Kissat solver for level X"**
   - Check Kissat library installation
   - Verify memory availability
   - Reduce number of concurrent solvers

2. **"Per-level Kissat integration requires SSAT solving"**
   - Add `-s` flag for SSAT problems
   - Use `-kissat` without `-kissat-per-level` for SAT problems

3. **Poor performance with universal quantifiers**
   - Universal variables have limited Kissat support
   - Consider using original SharpSSAT for universal-heavy problems
   - Monitor performance and adjust strategy

### Performance Tuning

1. **Adjust limits based on problem size:**
   ```bash
   # Small problems
   -kissat-conflict 100 -kissat-decision 200
   
   # Large problems  
   -kissat-conflict 10000 -kissat-decision 50000
   ```

2. **Use appropriate solving mode:**
   ```bash
   # SAT problems
   -kissat
   
   # SSAT problems
   -s -kissat -kissat-per-level
   ```

3. **Monitor resource usage:**
   - Set time limits to prevent hanging
   - Monitor memory usage with large problems
   - Use quiet mode (`-q`) for batch processing

## Future Enhancements

1. **Incremental Solving**: Support for incremental SAT solving across levels
2. **Adaptive Strategy**: Automatically choose best solving strategy
3. **Parallel Solving**: Parallel processing of independent levels
4. **Advanced Clause Filtering**: More sophisticated dependency analysis
5. **Integration with Other Solvers**: Support for additional SAT solvers

## Conclusion

The enhanced SharpSSAT-Kissat integration provides a powerful combination of SharpSSAT's advanced SSAT capabilities with Kissat's high-performance SAT solving. The per-quantifier-level approach enables efficient solving of complex SSAT problems while maintaining the flexibility to fall back to original SharpSSAT when needed.

For optimal results:
- Use `-kissat-per-level` for multi-level SSAT problems
- Set appropriate resource limits
- Monitor performance and adjust strategy
- Consider problem characteristics when choosing integration mode 