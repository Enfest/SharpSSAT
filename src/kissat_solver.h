#ifndef KISSAT_SOLVER_H
#define KISSAT_SOLVER_H

#include "solver.h"
#include "kissat_integration.h"
#include <vector>
#include <map>

// Extended solver configuration
struct KissatSolverConfiguration : public SolverConfiguration {
    bool use_kissat = false;           // Enable Kissat integration
    bool kissat_incremental = false;   // Use incremental solving with Kissat
    bool kissat_per_level = false;     // Use separate Kissat instance per quantifier level
    unsigned kissat_conflict_limit = 0; // Conflict limit for Kissat
    unsigned kissat_decision_limit = 0; // Decision limit for Kissat
    int kissat_time_limit = 0;         // Time limit for Kissat (seconds)
};

// Extended solver that can use Kissat for SAT solving
class KissatSolver : public Solver {
public:
    KissatSolver();
    ~KissatSolver();
    
    // Override configuration access
    KissatSolverConfiguration& config() {
        return kissat_config_;
    }
    
    // Initialize Kissat integration
    bool initKissat();
    
    // Override solve method to use Kissat when enabled
    bool solve_ind(const std::string& file_name);
    
    // Solve a CNF formula using Kissat
    int solveCNFWithKissat(const std::vector<std::vector<int>>& clauses);
    
    // Solve with assumptions using Kissat
    int solveWithAssumptions(const std::vector<std::vector<int>>& clauses,
                            const std::vector<int>& assumptions);
    
    // Convert SharpSSAT formula to CNF for Kissat
    std::vector<std::vector<int>> convertToCNF();
    
    // Get solution from Kissat
    std::vector<int> getKissatSolution() const;
    
    // Check if Kissat found a solution
    bool isKissatSatisfiable() const;
    
    // Check if Kissat found unsatisfiable
    bool isKissatUnsatisfiable() const;

private:
    KissatSolverConfiguration kissat_config_;
    KissatIntegration* kissat_integration_;
    
    // Per-level Kissat instances for SSAT solving
    std::map<int, KissatIntegration*> level_kissat_solvers_;
    
    // Use Kissat for BCP when enabled
    bool useKissatForBCP();
    
    // Use Kissat for SAT solving
    bool useKissatForSAT();
    
    // Use Kissat for SSAT solving with quantifier levels
    bool useKissatForSSAT();
    
    // Initialize Kissat solvers for each quantifier level
    bool initLevelKissatSolvers();
    
    // Get Kissat solver for a specific quantifier level
    KissatIntegration* getKissatSolverForLevel(int level);
    
    // Solve current formula with level-specific Kissat
    int solveWithLevelKissat(int level, const std::vector<std::vector<int>>& clauses);
    
    // Convert SharpSSAT literals to CNF format
    std::vector<int> convertLiteralsToCNF(const std::vector<LiteralID>& literals);
    
    // Convert CNF literals back to SharpSSAT format
    std::vector<LiteralID> convertCNFToLiterals(const std::vector<int>& cnf_literals);
    
    // Build CNF from current formula state
    void buildCNFFromFormula(std::vector<std::vector<int>>& cnf_clauses);
    
    // Build CNF from binary clauses
    void buildCNFFromBinaryClauses(std::vector<std::vector<int>>& cnf_clauses);
    
    // Build CNF from long clauses
    void buildCNFFromLongClauses(std::vector<std::vector<int>>& cnf_clauses);
    
    // Build CNF from unit clauses
    void buildCNFFromUnitClauses(std::vector<std::vector<int>>& cnf_clauses);
    
    // Apply Kissat solution to SharpSSAT state
    void applyKissatSolution(const std::vector<int>& solution);
    
    // Check if current formula is suitable for Kissat
    bool isFormulaSuitableForKissat();
    
    // Check if formula is suitable for level-specific Kissat
    bool isFormulaSuitableForLevelKissat(int level);
    
    // Convert SharpSSAT literal to CNF literal
    int toCNFLiteral(LiteralID lit) const;
    
    // Convert CNF literal to SharpSSAT literal
    LiteralID fromCNFLiteral(int cnf_lit) const;
    
    // Get variables at a specific quantifier level
    std::vector<VariableIndex> getVariablesAtLevel(int level);
    
    // Build CNF for a specific quantifier level
    std::vector<std::vector<int>> buildCNFForLevel(int level);
    
    // Apply level-specific solution
    void applyLevelSolution(int level, const std::vector<int>& solution);
};

#endif // KISSAT_SOLVER_H 