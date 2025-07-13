#include "kissat_integration.h"

// Include Kissat headers
extern "C" {
#include "../kissat/src/kissat.h"
}

#include <iostream>
#include <cassert>
#include <vector>

// KissatWrapper implementation
KissatWrapper::KissatWrapper() : solver_(nullptr), initialized_(false) {
    std::cout << "KissatWrapper constructor called" << std::endl;
}

KissatWrapper::~KissatWrapper() {
    std::cout << "KissatWrapper destructor called, solver_: " << solver_ << ", initialized_: " << initialized_ << std::endl;
    if (solver_) {
        std::cout << "Releasing solver in destructor" << std::endl;
        kissat_release(solver_);
        solver_ = nullptr;
        initialized_ = false;
    }
    std::cout << "KissatWrapper destructor completed" << std::endl;
}

bool KissatWrapper::init() {
    std::cout << "KissatWrapper::init() called" << std::endl;
    if (initialized_) {
        std::cout << "Already initialized" << std::endl;
        return true;
    }
    
    std::cout << "Calling kissat_init()" << std::endl;
    solver_ = kissat_init();
    kissat_set_option(solver_, "quiet", 1); // turn off kissat output
    if (!solver_) {
        std::cerr << "Failed to initialize Kissat solver" << std::endl;
        return false;
    }
    
    std::cout << "Kissat solver initialized successfully" << std::endl;
    initialized_ = true;
    return true;
}

void KissatWrapper::add_literal(int lit) {
    assert(initialized_ && solver_);
    try {
        kissat_add(solver_, lit);
    } catch (...) {
        std::cerr << "Exception in kissat_add with literal " << lit << std::endl;
        throw;
    }
}

void KissatWrapper::add_clause(const std::vector<int>& clause) {
    assert(initialized_ && solver_);
    for(int lit : clause) {
        kissat_add(solver_, lit);
    }
    kissat_add(solver_, 0);
}

void KissatWrapper::reserve(int max_var) {
    assert(initialized_ && solver_);
    kissat_reserve(solver_, max_var);
}

void KissatWrapper::finalize_clause() {
    assert(initialized_ && solver_);
    kissat_add(solver_, 0); // 0 terminates a clause in Kissat
}

int KissatWrapper::solve() {
    assert(initialized_ && solver_);
    return kissat_solve(solver_);
}

int KissatWrapper::get_value(int lit) {
    assert(initialized_ && solver_);
    return kissat_value(solver_, lit);
}

void KissatWrapper::add_assumptions(std::vector<int> lit) {
    assert(initialized_ && solver_);
    // Note: Kissat doesn't have a direct assumption API in the public interface
    // We'll need to implement this differently or use a workaround
    // For now, we'll add it as a unit clause
    for(int lit : lit) {
        kissat_add(solver_, lit);
    }
    kissat_add(solver_, 0);
}

void KissatWrapper::clear_assumptions() {
    // Reset the solver to clear assumptions
    reset();
}

void KissatWrapper::set_terminate_callback(void* state, int (*terminate)(void* state)) {
    assert(initialized_ && solver_);
    kissat_set_terminate(solver_, state, terminate);
}

void KissatWrapper::set_time_limit(int seconds) {
    std::cout << "KissatWrapper::set_time_limit(" << seconds << ") called" << std::endl;
    assert(initialized_ && solver_);
    // Kissat doesn't have a direct time limit API in the public interface
    // This would need to be implemented through the termination callback
    // For now, we'll leave this as a no-op since the termination callback
    // would need to be set up separately
    std::cout << "KissatWrapper::set_time_limit completed" << std::endl;
}

void KissatWrapper::set_conflict_limit(unsigned limit) {
    std::cout << "KissatWrapper::set_conflict_limit(" << limit << ") called" << std::endl;
    assert(initialized_ && solver_);
    kissat_set_conflict_limit(solver_, limit);
    std::cout << "KissatWrapper::set_conflict_limit completed" << std::endl;
}

void KissatWrapper::set_decision_limit(unsigned limit) {
    std::cout << "KissatWrapper::set_decision_limit(" << limit << ") called" << std::endl;
    assert(initialized_ && solver_);
    kissat_set_decision_limit(solver_, limit);
    std::cout << "KissatWrapper::set_decision_limit completed" << std::endl;
}

void KissatWrapper::print_statistics() {
    assert(initialized_ && solver_);
    kissat_print_statistics(solver_);
}

void KissatWrapper::reset() {
    std::cout << "KissatWrapper::reset() called" << std::endl;
    if (solver_) {
        std::cout << "Releasing existing solver" << std::endl;
        kissat_release(solver_);
        solver_ = nullptr;
        initialized_ = false;
    }
    std::cout << "Initializing new solver" << std::endl;
    solver_ = kissat_init();
    if (solver_) {
        kissat_set_option(solver_, "quiet", 1); // turn off kissat output
    }
    initialized_ = (solver_ != nullptr);
    std::cout << "Reset completed, initialized: " << initialized_ << std::endl;
}
