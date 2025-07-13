#include "cadical_integration.h"

// Include CaDiCaL headers
#include "../cadical/src/cadical.hpp"

#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include <thread>

// Termination callback state
struct TerminateState {
    std::chrono::steady_clock::time_point start_time;
    int time_limit_seconds;
    unsigned conflict_limit;
    unsigned decision_limit;
    unsigned current_conflicts;
    unsigned current_decisions;
};

// Termination callback function
static int terminate_callback(void* state) {
    TerminateState* ts = static_cast<TerminateState*>(state);
    
    // Check time limit
    if (ts->time_limit_seconds > 0) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - ts->start_time);
        if (elapsed.count() >= ts->time_limit_seconds) {
            return 1; // Terminate
        }
    }
    
    // Check conflict limit
    if (ts->conflict_limit > 0 && ts->current_conflicts >= ts->conflict_limit) {
        return 1; // Terminate
    }
    
    // Check decision limit
    if (ts->decision_limit > 0 && ts->current_decisions >= ts->decision_limit) {
        return 1; // Terminate
    }
    
    return 0; // Continue
}

// CaDiCaLWrapper implementation
CaDiCaLWrapper::CaDiCaLWrapper() : solver_(nullptr), initialized_(false), deleted_(false) {
    std::cout << "CaDiCaLWrapper constructor called" << std::endl;
}

CaDiCaLWrapper::~CaDiCaLWrapper() {
    std::cout << "CaDiCaLWrapper destructor called, solver_: " << solver_ << ", initialized_: " << initialized_ << ", deleted_: " << deleted_ << std::endl;
    if (solver_ && initialized_ && !deleted_) {
        std::cout << "Cleaning up solver in destructor" << std::endl;
        deleted_ = true;
        try {
            // Just delete the solver without calling any methods first
            // The issue might be that calling methods after certain operations
            // puts the solver in an invalid state
            delete solver_;
            std::cout << "Solver deleted successfully" << std::endl;
        } catch (...) {
            std::cerr << "Exception during solver deletion" << std::endl;
        }
        solver_ = nullptr;
        initialized_ = false;
    }
    std::cout << "CaDiCaLWrapper destructor completed" << std::endl;
}

bool CaDiCaLWrapper::init() {
    std::cout << "CaDiCaLWrapper::init() called" << std::endl;
    if (initialized_) {
        std::cout << "Already initialized" << std::endl;
        return true;
    }
    
    std::cout << "Creating new CaDiCaL solver" << std::endl;
    solver_ = new CaDiCaL::Solver();
    if (!solver_) {
        std::cerr << "Failed to create CaDiCaL solver" << std::endl;
        return false;
    }
    
    // Set quiet mode
    solver_->set("quiet", 1);
    
    std::cout << "CaDiCaL solver initialized successfully" << std::endl;
    initialized_ = true;
    return true;
}

void CaDiCaLWrapper::add_literal(int lit) {
    if (!initialized_ || !solver_ || deleted_) {
        std::cerr << "Error: CaDiCaLWrapper not properly initialized or deleted" << std::endl;
        throw std::runtime_error("CaDiCaLWrapper not initialized");
    }
    try {
        solver_->add(lit);
    } catch (...) {
        std::cerr << "Exception in CaDiCaL add with literal " << lit << std::endl;
        throw;
    }
}

void CaDiCaLWrapper::add_clause(const std::vector<int>& clause) {
    if (!initialized_ || !solver_) {
        std::cerr << "Error: CaDiCaLWrapper not properly initialized" << std::endl;
        throw std::runtime_error("CaDiCaLWrapper not initialized");
    }
    for (int lit : clause) {
        solver_->add(lit);
    }
    solver_->add(0); // Terminate clause
}

void CaDiCaLWrapper::reserve(int max_var) {
    assert(initialized_ && solver_);
    solver_->reserve(max_var);
}

void CaDiCaLWrapper::finalize_clause() {
    assert(initialized_ && solver_);
    solver_->add(0); // 0 terminates a clause in CaDiCaL
}

int CaDiCaLWrapper::solve() {
    assert(initialized_ && solver_);
    return solver_->solve();
}

int CaDiCaLWrapper::get_value(int lit) {
    assert(initialized_ && solver_);
    return solver_->val(lit);
}

void CaDiCaLWrapper::add_assumptions(const std::vector<int>& assumptions) {
    assert(initialized_ && solver_);
    for (int lit : assumptions) {
        solver_->assume(lit);
    }
}

void CaDiCaLWrapper::clear_assumptions() {
    assert(initialized_ && solver_);
    solver_->reset_assumptions();
}

void CaDiCaLWrapper::set_terminate_callback(void* state, int (*terminate)(void* state)) {
    assert(initialized_ && solver_);
    // CaDiCaL doesn't have a direct termination callback API like Kissat
    // This would need to be implemented through a custom terminator class
    // For now, we'll leave this as a no-op
    std::cout << "CaDiCaL termination callback not yet implemented" << std::endl;
}

void CaDiCaLWrapper::set_time_limit(int seconds) {
    std::cout << "CaDiCaLWrapper::set_time_limit(" << seconds << ") called" << std::endl;
    assert(initialized_ && solver_);
    // CaDiCaL doesn't have a direct time limit API
    // This would need to be implemented through a custom terminator
    // For now, we'll leave this as a no-op
    std::cout << "CaDiCaL time limit not yet implemented" << std::endl;
}

void CaDiCaLWrapper::set_conflict_limit(unsigned limit) {
    std::cout << "CaDiCaLWrapper::set_conflict_limit(" << limit << ") called" << std::endl;
    assert(initialized_ && solver_);
    if (limit > 0) {
        solver_->limit("conflicts", limit);
        std::cout << "CaDiCaL conflict limit set to " << limit << std::endl;
    }
}

void CaDiCaLWrapper::set_decision_limit(unsigned limit) {
    std::cout << "CaDiCaLWrapper::set_decision_limit(" << limit << ") called" << std::endl;
    assert(initialized_ && solver_);
    if (limit > 0) {
        solver_->limit("decisions", limit);
        std::cout << "CaDiCaL decision limit set to " << limit << std::endl;
    }
}

void CaDiCaLWrapper::print_statistics() {
    assert(initialized_ && solver_);
    solver_->statistics();
}

void CaDiCaLWrapper::reset() {
    std::cout << "CaDiCaLWrapper::reset() called" << std::endl;
    if (solver_ && initialized_ && !deleted_) {
        std::cout << "Cleaning up existing solver" << std::endl;
        deleted_ = true;
        try {
            // Just delete the solver without calling any methods first
            delete solver_;
        } catch (...) {
            std::cerr << "Exception during solver deletion in reset" << std::endl;
        }
        solver_ = nullptr;
        initialized_ = false;
    }
    deleted_ = false;  // Reset the deleted flag
    std::cout << "Creating new solver" << std::endl;
    try {
        solver_ = new CaDiCaL::Solver();
        if (solver_) {
            solver_->set("quiet", 1); // Set quiet mode
            initialized_ = true;
        } else {
            initialized_ = false;
        }
    } catch (...) {
        std::cerr << "Exception during solver creation in reset" << std::endl;
        solver_ = nullptr;
        initialized_ = false;
    }
    std::cout << "Reset completed, initialized: " << initialized_ << std::endl;
}

bool CaDiCaLWrapper::failed(int lit) {
    assert(initialized_ && solver_);
    return solver_->failed(lit);
}

// CaDiCaLIntegration implementation
CaDiCaLIntegration::CaDiCaLIntegration() : wrapper_(nullptr) {
    std::cout << "CaDiCaLIntegration constructor called" << std::endl;
}

CaDiCaLIntegration::~CaDiCaLIntegration() {
    std::cout << "CaDiCaLIntegration destructor called" << std::endl;
    if (wrapper_) {
        delete wrapper_;
        wrapper_ = nullptr;
    }
    std::cout << "CaDiCaLIntegration destructor completed" << std::endl;
}

bool CaDiCaLIntegration::init() {
    std::cout << "CaDiCaLIntegration::init() called" << std::endl;
    if (wrapper_) {
        std::cout << "Already initialized" << std::endl;
        return true;
    }
    
    wrapper_ = new CaDiCaLWrapper();
    if (!wrapper_->init()) {
        std::cerr << "Failed to initialize CaDiCaL wrapper" << std::endl;
        delete wrapper_;
        wrapper_ = nullptr;
        return false;
    }
    
    std::cout << "CaDiCaL integration initialized successfully" << std::endl;
    return true;
}

int CaDiCaLIntegration::solve_cnf(const std::vector<std::vector<int>>& clauses) {
    if (!wrapper_ || !wrapper_->is_initialized()) {
        std::cerr << "CaDiCaL integration not initialized" << std::endl;
        return -1;
    }
    
    // Reset solver for new formula
    wrapper_->reset();
    
    // Add all clauses
    for (const auto& clause : clauses) {
        wrapper_->add_clause(clause);
    }
    
    // Solve
    int result = wrapper_->solve();
    
    // Store solution if SAT
    if (result == CaDiCaL::SATISFIABLE) {
        last_solution_.clear();
        // Extract solution - we need to know the number of variables
        // For now, we'll assume the maximum variable number is the highest absolute value in clauses
        int max_var = 0;
        for (const auto& clause : clauses) {
            for (int lit : clause) {
                max_var = std::max(max_var, std::abs(lit));
            }
        }
        
        for (int var = 1; var <= max_var; ++var) {
            int val = wrapper_->get_value(var);
            last_solution_.push_back(val);
        }
    }
    
    return result;
}

int CaDiCaLIntegration::solve_with_assumptions(const std::vector<std::vector<int>>& clauses,
                                              const std::vector<int>& assumptions) {
    if (!wrapper_ || !wrapper_->is_initialized()) {
        std::cerr << "CaDiCaL integration not initialized" << std::endl;
        return -1;
    }
    
    // Reset solver for new formula
    wrapper_->reset();
    
    // Add all clauses
    for (const auto& clause : clauses) {
        wrapper_->add_clause(clause);
    }
    
    // Add assumptions
    wrapper_->add_assumptions(assumptions);
    
    // Solve
    int result = wrapper_->solve();
    
    // Store solution if SAT
    if (result == CaDiCaL::SATISFIABLE) {
        last_solution_.clear();
        // Extract solution - we need to know the number of variables
        int max_var = 0;
        for (const auto& clause : clauses) {
            for (int lit : clause) {
                max_var = std::max(max_var, std::abs(lit));
            }
        }
        
        for (int var = 1; var <= max_var; ++var) {
            int val = wrapper_->get_value(var);
            last_solution_.push_back(val);
        }
    }
    
    return result;
}

std::vector<int> CaDiCaLIntegration::get_solution() const {
    return last_solution_;
}

void CaDiCaLIntegration::set_conflict_limit(unsigned limit) {
    if (wrapper_) {
        wrapper_->set_conflict_limit(limit);
    }
}

void CaDiCaLIntegration::set_decision_limit(unsigned limit) {
    if (wrapper_) {
        wrapper_->set_decision_limit(limit);
    }
}

void CaDiCaLIntegration::set_time_limit(int seconds) {
    if (wrapper_) {
        wrapper_->set_time_limit(seconds);
    }
} 