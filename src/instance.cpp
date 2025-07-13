/*
 * instance.cpp
 *
 *  Created on: Aug 23, 2012
 *      Author: Marc Thurley
 */

#include "instance.h"


#include <algorithm>
#include <fstream>
#include <sys/stat.h>
#include <iostream>
#include <vector>
#include <map>


using namespace std;


// file parsing utility
float parseFloat(ifstream& in){
  string buf;
  in >> buf;
  return stof(buf);
}

void Instance::cleanClause(ClauseOfs cl_ofs) {
  bool satisfied = false;
  for (auto it = beginOf(cl_ofs); *it != SENTINEL_LIT; it++)
    if (isSatisfied(*it)) {
      satisfied = true;
      break;
    }
  // mark the clause as empty if satisfied
  if (satisfied) {
    *beginOf(cl_ofs) = SENTINEL_LIT;
    return;
  }
  auto jt = beginOf(cl_ofs);
  auto it = beginOf(cl_ofs);
  // from now, all inactive literals are resolved
  for (; *it != SENTINEL_LIT; it++, jt++) {
    while (*jt != SENTINEL_LIT && !isActive(*jt))
      jt++;
    *it = *jt;
    if (*jt == SENTINEL_LIT)
      break;
  }
  unsigned length = it - beginOf(cl_ofs);
  // if it has become a unit clause, it should have already been asserted
  if (length == 1) {
    *beginOf(cl_ofs) = SENTINEL_LIT;
    // if it has become binary, transform it to binary and delete it
  } else if (length == 2) {
    addBinaryClause(*beginOf(cl_ofs), *(beginOf(cl_ofs) + 1));
    *beginOf(cl_ofs) = SENTINEL_LIT;
  }
}

void Instance::compactClauses() {
  vector<ClauseOfs> clause_ofs;
  clause_ofs.reserve(statistics_.num_long_clauses_);

  // clear watch links and occurrence lists
  for (auto it_lit = literal_pool_.begin(); it_lit != literal_pool_.end();
      it_lit++) {
    if (*it_lit == SENTINEL_LIT) {
      if (it_lit + 1 == literal_pool_.end())
        break;
      it_lit += ClauseHeader::overheadInLits();
      clause_ofs.push_back(1 + it_lit - literal_pool_.begin());
    }
  }

  for (auto ofs : clause_ofs)
    cleanClause(ofs);

  for (auto &l : literals_)
    l.resetWatchList();

  occurrence_lists_.clear();
  occurrence_lists_.resize(variables_.size());

  vector<LiteralID> tmp_pool = literal_pool_;
  literal_pool_.clear();
  literal_pool_.push_back(SENTINEL_LIT);
  ClauseOfs new_ofs;
  unsigned num_clauses = 0;
  for (auto ofs : clause_ofs) {
    auto it = (tmp_pool.begin() + ofs);
    if (*it != SENTINEL_LIT) {
      for (unsigned i = 0; i < ClauseHeader::overheadInLits(); i++)
        literal_pool_.push_back(0);
      new_ofs = literal_pool_.size();
      literal(*it).addWatchLinkTo(new_ofs);
      literal(*(it + 1)).addWatchLinkTo(new_ofs);
      num_clauses++;
      for (; *it != SENTINEL_LIT; it++) {
        literal_pool_.push_back(*it);
        occurrence_lists_[*it].push_back(new_ofs);
      }
      literal_pool_.push_back(SENTINEL_LIT);
    }
  }

  vector<LiteralID> tmp_bin;
  unsigned bin_links = 0;
  for (auto &l : literals_) {
    tmp_bin.clear();
    for (auto it = l.binary_links_.begin(); *it != SENTINEL_LIT; it++)
      if (isActive(*it))
        tmp_bin.push_back(*it);
    bin_links += tmp_bin.size();
    tmp_bin.push_back(SENTINEL_LIT);
    l.binary_links_ = tmp_bin;
  }
  statistics_.num_long_clauses_ = num_clauses;
  statistics_.num_binary_clauses_ = bin_links >> 1;
}

void Instance::compactVariables() {
  vector<unsigned> var_map(variables_.size(), 0);
  unsigned last_ofs = 0;
  unsigned num_isolated = 0;
  LiteralIndexedVector<vector<LiteralID> > _tmp_bin_links(1);
  LiteralIndexedVector<TriValue> _tmp_values = literal_values_;

  for (auto l : literals_)
    _tmp_bin_links.push_back(l.binary_links_);

  assert(_tmp_bin_links.size() == literals_.size());
  for (unsigned v = 1; v < variables_.size(); v++)
    if (isActive(v)) {
      if (isolated(v)) {
        num_isolated++;
        continue;
      }
      last_ofs++;
      var_map[v] = last_ofs;
    }

  variables_.clear();
  variables_.resize(last_ofs + 1);
  occurrence_lists_.clear();
  occurrence_lists_.resize(variables_.size());
  literals_.clear();
  literals_.resize(variables_.size());
  // ssat FIXME for model count
  literal_values_.clear();
  literal_values_.resize(variables_.size(), X_TRI);

  unsigned bin_links = 0;
  LiteralID newlit;
  for (auto l = LiteralID(0, false); l != _tmp_bin_links.end_lit(); l.inc()) {
    if (var_map[l.var()] != 0) {
      newlit = LiteralID(var_map[l.var()], l.sign());
      for (auto it = _tmp_bin_links[l].begin(); *it != SENTINEL_LIT; it++) {
        assert(var_map[it->var()] != 0);
        literals_[newlit].addBinLinkTo(
            LiteralID(var_map[it->var()], it->sign()));
      }
      bin_links += literals_[newlit].binary_links_.size() - 1;
    }
  }

  vector<ClauseOfs> clause_ofs;
  clause_ofs.reserve(statistics_.num_long_clauses_);
  // clear watch links and occurrence lists
  for (auto it_lit = literal_pool_.begin(); it_lit != literal_pool_.end();
      it_lit++) {
    if (*it_lit == SENTINEL_LIT) {
      if (it_lit + 1 == literal_pool_.end())
        break;
      it_lit += ClauseHeader::overheadInLits();
      clause_ofs.push_back(1 + it_lit - literal_pool_.begin());
    }
  }

  for (auto ofs : clause_ofs) {
    literal(LiteralID(var_map[beginOf(ofs)->var()], beginOf(ofs)->sign())).addWatchLinkTo(
        ofs);
    literal(LiteralID(var_map[(beginOf(ofs) + 1)->var()],
            (beginOf(ofs) + 1)->sign())).addWatchLinkTo(ofs);
    for (auto it_lit = beginOf(ofs); *it_lit != SENTINEL_LIT; it_lit++) {
      *it_lit = LiteralID(var_map[it_lit->var()], it_lit->sign());
      occurrence_lists_[*it_lit].push_back(ofs);
    }
  }

  // ssat FIXME for model count
  literal_values_.clear();
  literal_values_.resize(variables_.size(), X_TRI);
  unit_clauses_.clear();

  statistics_.num_variables_ = variables_.size() - 1 + num_isolated;

  statistics_.num_used_variables_ = num_variables();
  statistics_.num_free_variables_ = num_isolated;
}

void Instance::compactConflictLiteralPool(){
  auto write_pos = conflict_clauses_begin();
  vector<ClauseOfs> tmp_conflict_clauses = conflict_clauses_;
  conflict_clauses_.clear();
  for(auto clause_ofs: tmp_conflict_clauses){
    auto read_pos = beginOf(clause_ofs) - ClauseHeader::overheadInLits();
    for(unsigned i = 0; i < ClauseHeader::overheadInLits(); i++)
      *(write_pos++) = *(read_pos++);
    ClauseOfs new_ofs =  write_pos - literal_pool_.begin();
    conflict_clauses_.push_back(new_ofs);
    // first substitute antecedent if clause_ofs implied something
    if(isAntecedentOf(clause_ofs, *beginOf(clause_ofs)))
      var(*beginOf(clause_ofs)).ante = Antecedent(new_ofs);

    // now redo the watches
    literal(*beginOf(clause_ofs)).replaceWatchLinkTo(clause_ofs,new_ofs);
    literal(*(beginOf(clause_ofs)+1)).replaceWatchLinkTo(clause_ofs,new_ofs);
    // next, copy clause data
    assert(read_pos == beginOf(clause_ofs));
    while(*read_pos != SENTINEL_LIT)
      *(write_pos++) = *(read_pos++);
    *(write_pos++) = SENTINEL_LIT;
  }
  literal_pool_.erase(write_pos,literal_pool_.end());
}


bool Instance::deleteConflictClauses() {
  statistics_.times_conflict_clauses_cleaned_++;
  if (conflict_clauses_.empty()) return true;
  vector<ClauseOfs> tmp_conflict_clauses = conflict_clauses_;
  conflict_clauses_.clear();
  vector<double> tmp_ratios;
  double score, lifetime;
  for(auto clause_ofs: tmp_conflict_clauses){
    score = getHeaderOf(clause_ofs).score();
    lifetime = statistics_.num_conflicts_ - getHeaderOf(clause_ofs).creation_time();
    tmp_ratios.push_back(score/lifetime/(getHeaderOf(clause_ofs).length()));
  }
  vector<double> tmp_ratiosB = tmp_ratios;

  sort(tmp_ratiosB.begin(), tmp_ratiosB.end());

  double cutoff = tmp_ratiosB[tmp_ratiosB.size()/2];

  for(unsigned i = 0; i < tmp_conflict_clauses.size(); i++){
    if(tmp_ratios[i] < cutoff){
      if(!markClauseDeleted(tmp_conflict_clauses[i]))
        conflict_clauses_.push_back(tmp_conflict_clauses[i]);
    } else
      conflict_clauses_.push_back(tmp_conflict_clauses[i]);
  }
  return true;
}


bool Instance::markClauseDeleted(ClauseOfs cl_ofs){
  // only first literal may possibly have cl_ofs as antecedent
  if(isAntecedentOf(cl_ofs, *beginOf(cl_ofs)))
    return false;

  literal(*beginOf(cl_ofs)).removeWatchLinkTo(cl_ofs);
  literal(*(beginOf(cl_ofs)+1)).removeWatchLinkTo(cl_ofs);
  return true;
}


bool Instance::createfromFile(const string &file_name, SolverConfiguration& config) {
  unsigned int nVars, nCls;
  int lit, var;
  float prob;
  unsigned max_ignore = 1000000;
  unsigned clauses_added = 0;
  unsigned qlev = 0;
  LiteralID llit;
  vector<LiteralID> literals;
  Vars vars;
  string idstring;
  char c;

  // clear everything
  literal_pool_.clear();
  literal_pool_.push_back(SENTINEL_LIT);

  variables_.clear();
  variables_.push_back(Variable()); //initializing the Sentinel
  literal_values_.clear();
  unit_clauses_.clear();

  ///BEGIN File input
  ifstream input_file(file_name);
  if (!input_file) {
    cerr << "Cannot open file: " << file_name << endl;
    return false;
  }

  struct stat filestatus;
  stat(file_name.c_str(), &filestatus);

  literals.reserve(10000);
  while (input_file >> c && c != 'p')
    input_file.ignore(max_ignore, '\n');
  if (!(input_file >> idstring && (idstring == "cnf" || idstring=="wcnf") && input_file >> nVars
      && input_file >> nCls)) {
    cerr << "Invalid CNF(WCNF) file" << endl;
    return false;
  }
  if(idstring=="wcnf") f_type_ = WCNF;

  variables_.resize(nVars + 1);
  literal_values_.resize(nVars + 1, X_TRI);
  literal_pool_.reserve(filestatus.st_size);
  conflict_clauses_.reserve(2*nCls);
  occurrence_lists_.clear();
  occurrence_lists_.resize(nVars + 1);


  literals_.clear();
  literals_.resize(nVars + 1);

  var2Prob_.clear();
  var2Prob_.resize(nVars + 1, -1);
  var2Q_.clear();
  var2Q_.resize(nVars + 1, EXISTENTIAL);
  var2Lev_.clear();
  var2Lev_.resize(nVars+1, -1);

  // clauses stored in CNF format for each quantifier level
  vector<vector<int>> all_clauses;


  while ((input_file >> c) && clauses_added < nCls) {
    if ((c == '-') || isdigit(c)) {
      input_file.unget();//extracted a nonspace character to determine if we have a clause, so put it back
      literals.clear();
      bool skip_clause = false;
      while ((input_file >> lit) && lit != 0) {
        bool duplicate_literal = false;
        for (auto i : literals) {
          if (i.toInt() == lit) {
            duplicate_literal = true;
            break;
          }
          if (i.toInt() == -lit) {
            skip_clause = true;
            break;
          }
        }
        if (!duplicate_literal) {
          literals.push_back(lit);
        }
      }
      if (!skip_clause) {
        assert(!literals.empty());
        clauses_added++;
        statistics_.incorporateClauseData(literals);
        ClauseOfs cl_ofs = addClause(literals);
        // FIXME
        if (literals.size() >= 3)
          for (auto l : literals){
            occurrence_lists_[l].push_back(cl_ofs);
          }

        vector<int> clause;
        // Add all literals to all_clauses for cadical
        for (auto l : literals) {
          clause.push_back(l.toInt());
        }
        all_clauses.push_back(clause);
      }
    }
    else if( c=='r' || c=='e' || c=='a' ){ // reading prefix for ssat
      vars.clear();
      QType qt;
      if(c=='r'){
        qt = RANDOM;
        prob = parseFloat(input_file);
        while ( (input_file >> var) && var!=0 ){
          vars.push_back(var);
          var2Prob_[var] = prob;
          var2Q_[var] = qt;
          orderedVar_.push_back(var);
        }
      }
      else if(c=='e'){
        qt = EXISTENTIAL;
        while( (input_file >> var) && var!=0  ){
          vars.push_back(var);
          var2Q_[var] = qt;
          orderedVar_.push_back(var);
        }
      }
      else{
        assert(c=='a');
        qt = UNIVERSAL;
        while( (input_file >> var) && var!=0  ){
          vars.push_back(var);
          //var2Prob_[var] = 0;
          var2Q_[var] = qt;
          orderedVar_.push_back(var);
        }
      }
      if(prefix_.empty()) {
        prefix_.push_back(QLevel(qt, vars));
        for(auto& v : vars) var2Lev_[v] = qlev;
      }
      else{
        QLevel& last = prefix_.back();
        if(last.first!=qt){
          prefix_.push_back( QLevel(qt, vars) );
          ++qlev;
          for(auto& v : vars) var2Lev_[v] = qlev;
        }
        else{
          last.second.insert(last.second.end(), vars.begin(), vars.end());
          for(auto& v : vars) var2Lev_[v] = qlev;
        }
      }
    }
    else if( c=='w' ){
      vars.clear();
      input_file >> var;
      prob = parseFloat(input_file);
      var2Prob_[var] = (var>0) ? prob : 1-prob;
      input_file >> var; assert(var==0); // dummy
    }
    else assert (c=='c');
    input_file.ignore(max_ignore, '\n');
  }
  ///END NEW
  input_file.close();
  if(!prefix_.empty()) f_type_ = SDIMACS;
  //  /// END FILE input

  statistics_.num_variables_ = statistics_.num_original_variables_ = nVars;
  statistics_.num_used_variables_ = num_variables();
  statistics_.num_free_variables_ = nVars - num_variables();
  statistics_.num_qlev = qlev;

  statistics_.num_original_clauses_ = nCls;

  statistics_.num_original_binary_clauses_ = statistics_.num_binary_clauses_;
  statistics_.num_original_unit_clauses_ = statistics_.num_unit_clauses_ =
      unit_clauses_.size();

  original_lit_pool_size_ = literal_pool_.size();

  for(size_t i = 0; i < statistics_.num_variables_; i++) {
    cout << "variable " << i << " has level " << var2Lev_[i] << endl;
  }
  
  // Track maximum variable for cadical solution extraction
  // max_variable_ = nVars;

  for (auto clause : all_clauses) {
    cout << "Clause: ";
    for (auto lit : clause) {
      cout << lit << " ";
    }
    cout << endl;
  }

  if(config.perform_independent_solving) {

    cout << "Performing independent solving" << endl;

    cout << "num_qlev: " << statistics_.num_qlev << endl;

    // Initialize var_assignment_
    var_assignment_.clear();
    // Size var_assignment_ to handle all possible variable indices
    // The component analyzer uses max_variable_id_ = literals.end_lit().var() - 1
    // which is literals.size()/2 - 1, so we need at least that many elements
    unsigned max_var_id = literals_.size() / 2;
    var_assignment_.resize(max_var_id + 1, -1);
    
    // Initialize cadical solvers for each quantifier level
    cout << "Initializing cadical solvers for " << statistics_.num_qlev << " levels..." << endl;
    level_cadical_solvers_.clear();
    level_cadical_solvers_.reserve(statistics_.num_qlev + 1);
    
    for(size_t i = 0; i < statistics_.num_qlev + 1; i++) {
      try {
        CaDiCaLWrapper* solver = new CaDiCaLWrapper();
        if (!solver) {
          cerr << "Failed to allocate CaDiCaLWrapper for level " << i << endl;
          return false;
        }

        if (!solver->init()) {
          cerr << "Failed to initialize cadical solver for level " << i << endl;
          delete solver;
          return false;
        }

        level_cadical_solvers_.push_back(solver);
        cout << "cadical solver " << i << " initialized successfully" << endl;

        if(config.cadical_conflict_limit > 0) {
          solver->set_conflict_limit(config.cadical_conflict_limit);
        }
        // else{
        //   solver->set_conflict_limit(1000000);
        // }
        if(config.cadical_decision_limit > 0) {
          solver->set_decision_limit(config.cadical_decision_limit);
        }
        // else{
        //   solver->set_decision_limit(1000000);
        // }

        
      } catch (const std::exception& e) {
        cerr << "Exception while creating cadical solver for level " << i << ": " << e.what() << endl;
        return false;
      } catch (...) {
        cerr << "Unknown exception while creating cadical solver for level " << i << endl;
        return false;
      }
    }

    cout << "Successfully initialized " << level_cadical_solvers_.size() << " cadical solvers" << endl;

    // Calculate maximum variable number for cadical reservation
    int max_cadical_var = 3*statistics_.num_variables_;
    
    // Check if we exceed cadical's maximum variable limit (1,073,741,823)
    const int CADICAL_MAX_VAR = 3*statistics_.num_variables_;
    if (max_cadical_var > CADICAL_MAX_VAR) {
        cerr << "Error: Maximum variable number " << max_cadical_var 
             << " exceeds cadical's limit of " << CADICAL_MAX_VAR << endl;
        cerr << "This problem is too large for cadical integration." << endl;
        return false;
    }
    
    // Reserve variables in all cadical solvers
    for(size_t i = 0; i < level_cadical_solvers_.size(); i++) {
        level_cadical_solvers_[i]->reserve(max_cadical_var);
        cout << "Reserved " << max_cadical_var << " variables in cadical solver " << i << endl;
    }

    // add clauses to cadical solvers
    for(size_t i = 0; i < statistics_.num_qlev + 1; i++) {

      cout << "Adding clauses to cadical solver for level " << i << endl;
      for(auto clause : all_clauses) {
        size_t max_level = 0;
        for(auto lit : clause) 
          if(var2Lev_[std::abs(lit)] > max_level) max_level = var2Lev_[std::abs(lit)];
        for(auto lit : clause) {
          cout << lit << " ";
        }
        cout << endl;
        level_cadical_solvers_[i]->add_clause(clause);
        if(i <= max_level) {
          for(auto lit : clause) {
            if(var2Lev_[std::abs(lit)] >= i) {
              int new_lit = lit > 0? lit+statistics_.num_variables_ : lit-statistics_.num_variables_;
              level_cadical_solvers_[i]->add_literal(new_lit);
              cout << new_lit << " ";
            }
            else{
              level_cadical_solvers_[i]->add_literal(lit);
              cout << lit << " ";
            }
          }
          cout << endl;
          level_cadical_solvers_[i]->add_literal(0);
        }
      }

    }

    // add zi -> (xi == yi) to cadical solver for level i
    // zi -> (xi + -yi)(-xi + yi) => -zi + (xi + -yi)(-xi + yi)
    // (-zi + xi + -yi)(-zi + -xi + yi)

    cout << "Adding zi -> (xi == yi) to cadical solvers" << endl;
    cout << "level_cadical_solvers_.size(): " << level_cadical_solvers_.size() << endl;

    for(size_t i=1; i<=statistics_.num_variables_; i++) {
      int solver_index = var2Lev_[i];
      // Skip variables that don't have a valid level (not in prefix)
      if (solver_index < 0) {
        cout << "Skipping variable " << i << " (no level assigned)" << endl;
        continue;
      }
      
      // Check bounds
      if (solver_index >= (int)level_cadical_solvers_.size()) {
        cerr << "Error: solver_index " << solver_index << " out of bounds for variable " << i << endl;
        return false;
      }
      
      vector<int> clause1 = {-(int)(i+statistics_.num_variables_*2), (int)i, -(int)(i+statistics_.num_variables_)};
      vector<int> clause2 = {-(int)(i+statistics_.num_variables_*2), -(int)i, (int)(i+statistics_.num_variables_)};

      level_cadical_solvers_[solver_index]->add_clause(clause1);
      level_cadical_solvers_[solver_index]->add_clause(clause2);

      cout << "Added clause to cadical solver for level " << solver_index << endl;
      cout << "Clause: " << -(int)((i+statistics_.num_variables_*2)) << " " << (int)i << " " << -(int)(i+statistics_.num_variables_) << " " << 0 << endl;
      cout << "Clause: " << -(int)(i+statistics_.num_variables_*2) << " " << -(int)i << " " << (int)(i+statistics_.num_variables_) << " " << 0 << endl;
    }

    // solve each level
  }
  cout << "About to return from createfromFile" << endl;

  return true;
}

