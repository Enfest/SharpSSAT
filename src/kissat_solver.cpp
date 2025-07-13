#include "kissat_solver.h"
#include <iostream>
#include <algorithm>

KissatSolver::KissatSolver() : Solver(), kissat_integration_(nullptr) {
    kissat_integration_ = new KissatIntegration();
}

KissatSolver::~KissatSolver() {
    if (kissat_integration_) {
        delete kissat_integration_;
    }
    
    // Clean up level-specific solvers
    for (auto& pair : level_kissat_solvers_) {
        if (pair.second) {
            delete pair.second;
        }
    }
    level_kissat_solvers_.clear();
}

bool KissatSolver::initKissat() {
    if (!kissat_config_.use_kissat) {
        return true;
    }
    
    // Initialize main Kissat integration
    if (!kissat_integration_->init()) {
        std::cerr << "Failed to initialize main Kissat integration" << std::endl;
        return false;
    }
    
    // Set Kissat limits if specified
    if (kissat_config_.kissat_conflict_limit > 0) {
        kissat_integration_->set_conflict_limit(kissat_config_.kissat_conflict_limit);
    }
    
    if (kissat_config_.kissat_decision_limit > 0) {
        kissat_integration_->set_decision_limit(kissat_config_.kissat_decision_limit);
    }
    
    if (kissat_config_.kissat_time_limit > 0) {
        kissat_integration_->set_time_limit(kissat_config_.kissat_time_limit);
    }
    
    // Initialize level-specific solvers if enabled
    if (kissat_config_.kissat_per_level && kissat_config_.ssat_solving) {
        if (!initLevelKissatSolvers()) {
            std::cerr << "Failed to initialize level-specific Kissat solvers" << std::endl;
            return false;
        }
    }
    
    return true;
}

bool KissatSolver::initLevelKissatSolvers() {
    // Clear existing level solvers
    for (auto& pair : level_kissat_solvers_) {
        if (pair.second) {
            delete pair.second;
        }
    }
    level_kissat_solvers_.clear();
    
    // Create solvers for each quantifier level
    for (int level = 0; level <= statistics_.num_qlev; ++level) {
        KissatIntegration* level_solver = new KissatIntegration();
        if (!level_solver->init()) {
            std::cerr << "Failed to initialize Kissat solver for level " << level << std::endl;
            delete level_solver;
            return false;
        }
        
        // Set limits for level-specific solver
        if (kissat_config_.kissat_conflict_limit > 0) {
            level_solver->set_conflict_limit(kissat_config_.kissat_conflict_limit);
        }
        
        if (kissat_config_.kissat_decision_limit > 0) {
            level_solver->set_decision_limit(kissat_config_.kissat_decision_limit);
        }
        
        if (kissat_config_.kissat_time_limit > 0) {
            level_solver->set_time_limit(kissat_config_.kissat_time_limit);
        }
        
        level_kissat_solvers_[level] = level_solver;
    }
    
    return true;
}

KissatIntegration* KissatSolver::getKissatSolverForLevel(int level) {
    auto it = level_kissat_solvers_.find(level);
    if (it != level_kissat_solvers_.end()) {
        return it->second;
    }
    return nullptr;
}

bool KissatSolver::solve_ind(const std::string& file_name) {
    // Initialize Kissat if enabled
    if (kissat_config_.use_kissat) {
        if (!initKissat()) {
            return false;
        }
    }
    
    // Call parent solve method using scope resolution
    return solve(file_name);
}

int KissatSolver::solveCNFWithKissat(const std::vector<std::vector<int>>& clauses) {
    if (!kissat_config_.use_kissat || !kissat_integration_) {
        return -1;
    }
    
    return kissat_integration_->solve_cnf(clauses);
}

int KissatSolver::solveWithAssumptions(const std::vector<std::vector<int>>& clauses,
                                      const std::vector<int>& assumptions) {
    if (!kissat_config_.use_kissat || !kissat_integration_) {
        return -1;
    }
    
    return kissat_integration_->solve_with_assumptions(clauses, assumptions);
}

int KissatSolver::solveWithLevelKissat(int level, const std::vector<std::vector<int>>& clauses) {
    KissatIntegration* level_solver = getKissatSolverForLevel(level);
    if (!level_solver) {
        return -1;
    }
    
    return level_solver->solve_cnf(clauses);
}

std::vector<std::vector<int>> KissatSolver::convertToCNF() {
    std::vector<std::vector<int>> cnf_clauses;
    
    // Build CNF from current formula state
    buildCNFFromFormula(cnf_clauses);
    
    return cnf_clauses;
}

std::vector<int> KissatSolver::getKissatSolution() const {
    if (!kissat_integration_) {
        return std::vector<int>();
    }
    
    return kissat_integration_->get_solution();
}

bool KissatSolver::isKissatSatisfiable() const {
    if (!kissat_integration_) {
        return false;
    }
    
    return kissat_integration_->is_satisfiable();
}

bool KissatSolver::isKissatUnsatisfiable() const {
    if (!kissat_integration_) {
        return false;
    }
    
    return kissat_integration_->is_unsatisfiable();
}

bool KissatSolver::useKissatForBCP() {
    // If Kissat is enabled and formula is suitable, use Kissat for BCP
    if (kissat_config_.use_kissat && isFormulaSuitableForKissat()) {
        if (kissat_config_.ssat_solving && kissat_config_.kissat_per_level) {
            return useKissatForSSAT();
        } else {
            return useKissatForSAT();
        }
    }
    
    return false; // Fall back to original BCP
}

bool KissatSolver::useKissatForSAT() {
    // Convert current formula to CNF
    std::vector<std::vector<int>> cnf_clauses = convertToCNF();
    
    if (cnf_clauses.empty()) {
        return true; // Empty formula is trivially satisfiable
    }
    
    // Solve with Kissat
    int result = kissat_integration_->solve_cnf(cnf_clauses);
    
    if (result == 10) { // SAT
        // Apply Kissat solution to SharpSSAT state
        std::vector<int> solution = kissat_integration_->get_solution();
        applyKissatSolution(solution);
        return true;
    } else if (result == 20) { // UNSAT
        return false;
    } else { // Unknown or error
        // Fall back to original solving method
        return false;
    }
}

bool KissatSolver::useKissatForSSAT() {
    // For SSAT, we need to solve level by level
    // Start from the innermost level (level 0)
    for (int level = 0; level <= statistics_.num_qlev; ++level) {
        if (!isFormulaSuitableForLevelKissat(level)) {
            continue;
        }
        
        // Build CNF for this level
        std::vector<std::vector<int>> level_cnf = buildCNFForLevel(level);
        
        if (level_cnf.empty()) {
            continue; // Skip empty levels
        }
        
        // Solve with level-specific Kissat
        int result = solveWithLevelKissat(level, level_cnf);
        
        if (result == 10) { // SAT
            // Get solution from level solver
            KissatIntegration* level_solver = getKissatSolverForLevel(level);
            if (level_solver) {
                std::vector<int> solution = level_solver->get_solution();
                applyLevelSolution(level, solution);
            }
            return true;
        } else if (result == 20) { // UNSAT
            return false;
        }
        // For unknown/error, continue to next level
    }
    
    // If no level-specific solving worked, fall back to general approach
    return useKissatForSAT();
}

std::vector<VariableIndex> KissatSolver::getVariablesAtLevel(int level) {
    std::vector<VariableIndex> vars;
    
    for (VariableIndex v = 1; v <= num_variables(); ++v) {
        if (qlev(v) == level && isActive(v)) {
            vars.push_back(v);
        }
    }
    
    return vars;
}

std::vector<std::vector<int>> KissatSolver::buildCNFForLevel(int level) {
    std::vector<std::vector<int>> cnf_clauses;
    
    // Get variables at this level
    std::vector<VariableIndex> level_vars = getVariablesAtLevel(level);
    
    if (level_vars.empty()) {
        return cnf_clauses; // No variables at this level
    }
    
    // Build clauses that involve variables at this level
    // This is a simplified approach - in practice, you'd need more sophisticated
    // clause filtering based on variable dependencies
    
    // Add unit clauses for this level
    for (const auto& lit : unit_clauses_) {
        if (isActive(lit) && qlev(lit) == level) {
            std::vector<int> clause = {toCNFLiteral(lit)};
            cnf_clauses.push_back(clause);
        }
    }
    
    // Add binary clauses for this level
    for (LiteralID l(1, false); l != literals_.end_lit(); l.inc()) {
        if (!isActive(l) || qlev(l) != level) continue;
        
        for (auto bt = literal(l).binary_links_.begin(); *bt != SENTINEL_LIT; bt++) {
            if (isActive(*bt) && qlev(*bt) == level && l.raw() < bt->raw()) {
                std::vector<int> clause = {toCNFLiteral(l), toCNFLiteral(*bt)};
                cnf_clauses.push_back(clause);
            }
        }
    }
    
    // Add long clauses that contain variables from this level
    for (auto it_lit = literal_pool_.begin(); it_lit != literal_pool_.end(); it_lit++) {
        if (*it_lit == SENTINEL_LIT) {
            if (it_lit + 1 != literal_pool_.end()) {
                it_lit += ClauseHeader::overheadInLits();
            }
            continue;
        }
        
        std::vector<int> clause;
        bool has_level_var = false;
        
        for (; *it_lit != SENTINEL_LIT; it_lit++) {
            if (isActive(*it_lit)) {
                clause.push_back(toCNFLiteral(*it_lit));
                if (qlev(*it_lit) == level) {
                    has_level_var = true;
                }
            }
        }
        
        if (!clause.empty() && has_level_var) {
            cnf_clauses.push_back(clause);
        }
    }
    
    return cnf_clauses;
}

void KissatSolver::applyLevelSolution(int level, const std::vector<int>& solution) {
    // Apply the solution to variables at the specific level
    for (int cnf_lit : solution) {
        LiteralID lit = fromCNFLiteral(cnf_lit);
        if (lit.var() <= num_variables() && qlev(lit) == level) {
            literal_values_[lit] = T_TRI;
            literal_values_[lit.neg()] = F_TRI;
        }
    }
}

std::vector<int> KissatSolver::convertLiteralsToCNF(const std::vector<LiteralID>& literals) {
    std::vector<int> cnf_literals;
    cnf_literals.reserve(literals.size());
    
    for (const auto& lit : literals) {
        cnf_literals.push_back(toCNFLiteral(lit));
    }
    
    return cnf_literals;
}

std::vector<LiteralID> KissatSolver::convertCNFToLiterals(const std::vector<int>& cnf_literals) {
    std::vector<LiteralID> literals;
    literals.reserve(cnf_literals.size());
    
    for (int cnf_lit : cnf_literals) {
        literals.push_back(fromCNFLiteral(cnf_lit));
    }
    
    return literals;
}

void KissatSolver::buildCNFFromFormula(std::vector<std::vector<int>>& cnf_clauses) {
    // Clear existing clauses
    cnf_clauses.clear();
    
    // Build CNF from different clause types
    buildCNFFromUnitClauses(cnf_clauses);
    buildCNFFromBinaryClauses(cnf_clauses);
    buildCNFFromLongClauses(cnf_clauses);
}

void KissatSolver::buildCNFFromBinaryClauses(std::vector<std::vector<int>>& cnf_clauses) {
    // Iterate through all literals to find binary clauses
    for (LiteralID l(1, false); l != literals_.end_lit(); l.inc()) {
        if (!isActive(l)) continue;
        
        // Check binary links
        for (auto bt = literal(l).binary_links_.begin(); *bt != SENTINEL_LIT; bt++) {
            if (isActive(*bt) && l.raw() < bt->raw()) { // Avoid duplicates
                std::vector<int> clause = {toCNFLiteral(l), toCNFLiteral(*bt)};
                cnf_clauses.push_back(clause);
            }
        }
    }
}

void KissatSolver::buildCNFFromLongClauses(std::vector<std::vector<int>>& cnf_clauses) {
    // Iterate through literal pool to find long clauses
    for (auto it_lit = literal_pool_.begin(); it_lit != literal_pool_.end(); it_lit++) {
        if (*it_lit == SENTINEL_LIT) {
            // End of clause
            if (it_lit + 1 != literal_pool_.end()) {
                it_lit += ClauseHeader::overheadInLits();
            }
            continue;
        }
        
        // Start of clause
        std::vector<int> clause;
        for (; *it_lit != SENTINEL_LIT; it_lit++) {
            if (isActive(*it_lit)) {
                clause.push_back(toCNFLiteral(*it_lit));
            }
        }
        
        if (!clause.empty()) {
            cnf_clauses.push_back(clause);
        }
    }
}

void KissatSolver::buildCNFFromUnitClauses(std::vector<std::vector<int>>& cnf_clauses) {
    // Add unit clauses
    for (const auto& lit : unit_clauses_) {
        if (isActive(lit)) {
            std::vector<int> clause = {toCNFLiteral(lit)};
            cnf_clauses.push_back(clause);
        }
    }
}

void KissatSolver::applyKissatSolution(const std::vector<int>& solution) {
    // Apply the solution to the SharpSSAT literal values
    for (int cnf_lit : solution) {
        LiteralID lit = fromCNFLiteral(cnf_lit);
        if (lit.var() <= num_variables()) {
            literal_values_[lit] = T_TRI;
            literal_values_[lit.neg()] = F_TRI;
        }
    }
}

bool KissatSolver::isFormulaSuitableForKissat() {
    // Check if the formula is suitable for Kissat solving
    // For now, we'll use Kissat for any formula, but this could be made more sophisticated
    
    // Don't use Kissat if we have universal quantifiers (unless configured otherwise)
    if (kissat_config_.include_forall) {
        return false;
    }
    
    // Don't use Kissat if we're doing SSAT solving (unless configured otherwise)
    if (kissat_config_.ssat_solving) {
        return false;
    }
    
    return true;
}

bool KissatSolver::isFormulaSuitableForLevelKissat(int level) {
    // Check if the formula at this level is suitable for Kissat solving
    
    // Get variables at this level
    std::vector<VariableIndex> level_vars = getVariablesAtLevel(level);
    
    if (level_vars.empty()) {
        return false; // No variables at this level
    }
    
    // Check if any variables at this level are universal
    for (VariableIndex v : level_vars) {
        if (qType(v) == UNIVERSAL) {
            return false; // Don't use Kissat for universal variables
        }
    }
    
    return true;
}

int KissatSolver::toCNFLiteral(LiteralID lit) const {
    // Convert SharpSSAT literal to CNF literal
    // SharpSSAT uses 1-based indexing, CNF typically uses 1-based
    int var = lit.var();
    return lit.sign() ? var : -var;
}

LiteralID KissatSolver::fromCNFLiteral(int cnf_lit) const {
    // Convert CNF literal to SharpSSAT literal
    int var = std::abs(cnf_lit);
    bool sign = cnf_lit > 0;
    return LiteralID(var, sign);
} 