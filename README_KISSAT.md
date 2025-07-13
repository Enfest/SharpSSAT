# SharpSSAT with Kissat Integration

This is an enhanced version of SharpSSAT that integrates with the Kissat SAT solver for improved performance on certain problem instances.

## Overview

SharpSSAT is a stochastic Boolean satisfiability (SSAT) solver that can now optionally use Kissat as its underlying SAT solver. This integration provides:

- **Improved SAT solving performance**: Kissat is a highly optimized SAT solver that can handle large CNF formulas efficiently
- **Better scalability**: For certain problem types, Kissat may provide better performance than the default SharpSAT engine
- **Configurable limits**: Set conflict, decision, and time limits for Kissat
- **Incremental solving**: Support for incremental SAT solving with Kissat

## Building

### Prerequisites

- GCC with C++11 support
- GMP library (libgmp-dev)
- Make

### Build Instructions

1. **Build Kissat library**:
   ```bash
   cd kissat
   ./configure
   make
   cd ..
   ```

2. **Build SharpSSAT with Kissat integration**:
   ```bash
   cd SharpSSAT
   make -f makefile_kissat
   ```

This will create the `SharpSSAT-Kissat` executable.

## Usage

### Basic Usage

```bash
./SharpSSAT-Kissat [options] <SDIMACS_File>
```

### Kissat-Specific Options

- `--kissat`: Enable Kissat integration
- `--kissat-conflicts <n>`: Set Kissat conflict limit
- `--kissat-decisions <n>`: Set Kissat decision limit  
- `--kissat-time <s>`: Set Kissat time limit (seconds)
- `--kissat-incremental`: Enable incremental solving with Kissat

### Example Commands

**Basic SSAT solving with Kissat**:
```bash
./SharpSSAT-Kissat --kissat -s input.sdimacs
```

**SSAT solving with Kissat limits**:
```bash
./SharpSSAT-Kissat --kissat --kissat-conflicts 10000 --kissat-time 300 -s input.sdimacs
```

**Strategy generation with Kissat**:
```bash
./SharpSSAT-Kissat --kissat -s -k input.sdimacs
```

**Certificate generation with Kissat**:
```bash
./SharpSSAT-Kissat --kissat -s -l input.sdimacs
```

## When to Use Kissat Integration

The Kissat integration is most beneficial for:

1. **Large CNF formulas**: When the underlying SAT instances are large and complex
2. **Hard SAT instances**: When the default SharpSAT engine struggles with certain problem types
3. **Time-critical applications**: When you need to set strict time limits
4. **Incremental solving**: When you need to solve multiple related SAT instances

## Limitations

- **Universal quantifiers**: Kissat integration is not available when using universal quantifiers (`-u` option)
- **SSAT solving**: Kissat integration may not be optimal for all SSAT problem types
- **Memory usage**: Kissat may use more memory than the default SharpSAT engine

## Configuration

The Kissat integration can be configured through the `KissatSolverConfiguration` struct:

```cpp
struct KissatSolverConfiguration : public SolverConfiguration {
    bool use_kissat = false;           // Enable Kissat integration
    bool kissat_incremental = false;   // Use incremental solving
    unsigned kissat_conflict_limit = 0; // Conflict limit
    unsigned kissat_decision_limit = 0; // Decision limit
    int kissat_time_limit = 0;         // Time limit (seconds)
};
```

## Architecture

The integration consists of several components:

1. **KissatIntegration**: High-level interface to Kissat
2. **KissatWrapper**: C++ wrapper around Kissat's C API
3. **KissatSolver**: Extended SharpSSAT solver that can use Kissat
4. **main_kissat.cpp**: Main entry point with Kissat-specific options

### File Structure

```
src/
├── kissat_integration.h      # Kissat integration interface
├── kissat_integration.cpp    # Kissat integration implementation
├── kissat_solver.h          # Extended solver with Kissat support
├── kissat_solver.cpp        # Extended solver implementation
└── main_kissat.cpp          # Main entry point with Kissat options
```

## Performance Considerations

- **Formula conversion overhead**: Converting SharpSSAT formulas to CNF format has some overhead
- **Memory usage**: Kissat may use different memory management strategies
- **Solver selection**: The integration automatically decides when to use Kissat vs. the default engine

## Troubleshooting

### Build Issues

1. **Kissat not found**: Ensure Kissat is built in the `../kissat` directory
2. **GMP library missing**: Install `libgmp-dev` package
3. **Compilation errors**: Ensure GCC supports C++11

### Runtime Issues

1. **Kissat initialization failed**: Check if Kissat library is properly linked
2. **Performance degradation**: Try disabling Kissat integration for certain problem types
3. **Memory issues**: Adjust Kissat limits or use the default engine

## Comparison with Original SharpSSAT

| Feature | Original SharpSSAT | SharpSSAT-Kissat |
|---------|-------------------|------------------|
| SAT Engine | SharpSAT | Kissat (optional) |
| Performance | Good for most cases | Better for large CNF |
| Memory Usage | Moderate | Variable |
| Universal Quantifiers | Full support | Limited with Kissat |
| Configuration | Basic | Extended with Kissat options |

## Future Enhancements

- **Hybrid solving**: Automatic selection between SharpSAT and Kissat based on problem characteristics
- **Parallel solving**: Use both engines in parallel and take the best result
- **Advanced heuristics**: Problem-specific heuristics for Kissat configuration
- **Benchmarking tools**: Tools to compare performance between engines

## References

- [SharpSSAT Paper](https://ojs.aaai.org/index.php/AAAI/article/view/25509)
- [Kissat SAT Solver](https://github.com/arminbiere/kissat)
- [SharpSAT](https://github.com/marcthurley/sharpSAT) 