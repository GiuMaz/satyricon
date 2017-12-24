#include "sat_solver.hpp"
#include "vsids.hpp"
#include "clause.hpp"
#include "assert_message.hpp"
#include <array>
#include <queue>
#include <algorithm>
#include <tuple>
#include <iomanip>
#include "log.hpp"

using namespace std;
using namespace Utils;

namespace Satyricon {

SATSolver::SATSolver():
    clauses(),
    learned(),
    number_of_variable(0),
    watch_list(),
    values(),
    decision_levels(),
    antecedents(),
    conflict_clause(nullptr),
    propagation_queue(),
    number_of_assigned_variable(),
    vsids(),
    trial(),
    current_level(0),
    log(Utils::LOG_NONE),
    model(),
    subsumption(),
    enable_preprocessing(true),
    enable_restart(true),
    enable_deletion(true),
    restart_interval_multiplier(10),
    restart_threshold(1),
    clause_activity_update(1.0),
    clause_decay_factor(1.001)
{}

void SATSolver::print_status(unsigned int conflict, unsigned int restart, unsigned int learn_limit) {
    log.normal << "conflict: " << setw(7) << conflict;
    if ( enable_restart )
        log.normal << ", restart: " << setw(7) <<  restart;
    if ( enable_deletion )
        log.normal << ", learn limit: " << setw(7) <<  learn_limit;
    log.normal << ", learned: " << setw(7) <<  learned.size() << endl;
}

void SATSolver::set_restarting_multiplier(unsigned int b) {
    restart_interval_multiplier = b;
}

void SATSolver::set_preprocessing(bool p) {
    enable_preprocessing = p;
}

void SATSolver::set_restart(bool r) {
    enable_restart = r;
}

void SATSolver::set_deletion(bool d) {
    enable_deletion = d;
}

int SATSolver::next_restart_interval() {

    if ( luby_next == ( (1<<luby_k) -1) ) {
        luby_memoization.push_back( 1 << (luby_k-1) );
        luby_k++;
    }
    else {
        luby_memoization.push_back(
                luby_memoization[luby_next - (1<< (luby_k-1))] );
    }

    luby_next++;

    return luby_memoization.back();
}

int SATSolver::new_restart_threshold() {
    return restart_interval_multiplier * next_restart_interval();
}

bool SATSolver::solve() {
    log.normal << "begin solve" << endl;
    unsigned int conflict_counter = 0;
    int restart_counter = 0;

    // maximum number of learned clause
    unsigned int learn_limit = clauses.size()*2;

    if ( enable_preprocessing) preprocessing();
    restart_threshold = new_restart_threshold();

    while ( true ) {

        log.verbose << "propagate at level " << current_level << endl;
        bool conflict = unit_propagation();
        if ( conflict ) {
            conflict_counter++;
            if ( conflict_counter % 1000 == 0 )
                print_status(conflict_counter,restart_counter, learn_limit);

            if ( current_level == 0 ) {
                log.verbose << "conflict at level 0, build unsat proof\n";
                build_unsat_proof();
                log.normal << "number of restart: " << restart_counter<< endl;
                return false; // UNSAT
            }

            auto backtrack_level  = conflict_analysis();
            backtrack( backtrack_level );
            learn_clause();

            // decay activity
            vsids.decay();
            clause_activity_decay();

        }
        else {
            if ( number_of_assigned_variable == number_of_variable ) {
                log.verbose << "assinged all literals without conflict\n";
                build_model();
                log.normal << "number of restart: " << restart_counter<< endl;
                return true; // SAT
            }

            if ( enable_deletion && learned.size() >= learn_limit ) {
                learn_limit+= (learn_limit/2); // increase the learning limit
                reduce_learned(); // remove low activity clause
            }

            if ( enable_restart && conflict_counter >= restart_threshold ) {
                restart_threshold += new_restart_threshold();
                restart_counter++;
                log.verbose << "restarting. next restart at " << restart_threshold << endl;
                backtrack(0);
            }

            // otherwise select a new literal
            current_level++;
            Literal l = decide_new_literal();
            log.verbose << "decide literal " << l << endl;
            assign( l, nullptr );
        }
    }
}

void SATSolver::build_model() {
    model.clear();
    int val = 1;
    for ( const auto& v : values) {
        assert_message(v != LIT_UNASIGNED,
                "Building a model with unsasigned literal");
        model.push_back( v == LIT_TRUE ? val : -val );
        val++;
    }
}

const vector<int>& SATSolver::get_model() {
    return model;
}

string SATSolver::string_model() {
    ostringstream oss;
    oss << "[ ";
    for ( auto i : model )
        oss << i << " ";
    oss << "]";
    return oss.str();
}

void SATSolver::set_log( Utils::Log l) {
    log = l;
}

literal_value SATSolver::get_asigned_value(const Literal & l) {
    switch (values[l.atom()]) {
        case LIT_TRUE:
            return l.is_negated() ? LIT_FALSE : LIT_TRUE;
        case LIT_FALSE:
            return l.is_negated() ? LIT_TRUE  : LIT_FALSE;
        default:
            return LIT_UNASIGNED;
    }
}

bool SATSolver::assign(Literal l, shared_ptr<SATSolver::Clause> antecedent) {

    if ( get_asigned_value(l) == LIT_TRUE )
        return false; // already assigned, no conflict
    if ( get_asigned_value(l) == LIT_FALSE )
        return true; // conflict!

    ++number_of_assigned_variable;
    log.verbose << "\tassign literal " << l <<
        " with level " << current_level <<
        " and antecedent " << (antecedent == nullptr ? "NONE" :  antecedent->print() ) << endl;
    values[l.atom()] = l.is_negated() ? LIT_FALSE : LIT_TRUE;
    decision_levels[l.atom()] = current_level;
    antecedents[l.atom()] = antecedent;

    // push the current decision, for eventual backtrack
    trial.push_back(l);

    // the assignment effect must propagate
    propagation_queue.push(l);

    return false; // no conflict found
}

bool SATSolver::unit_propagation() {
    while ( ! propagation_queue.empty() ) {
        // fetch an element from the queue
        auto l = propagation_queue.front(); propagation_queue.pop();
        log.verbose << "propagate " << l << endl;
        

        // extract the list of the opposite literal 
        // (is false now, their watcher must be moved)
        auto failed = !l;
        list<shared_ptr<Clause> > to_move;
        swap(to_move,watch_list[failed]);

        for (auto it = to_move.begin(); it != to_move.end(); ++it) {

            bool conflict = (*it)->propagate(failed);
            if ( ! conflict ) continue; // no problem, move to the next

            // CONFLICT
            log.verbose << "\tfound a conflict on clause " << (*it)->print() << endl;
            // initialize the conflict clause
            conflict_clause = (*it);
            // reset the other literal to move
            copy(it, to_move.end(), back_inserter(watch_list[failed]));
            // clear the propagation
            queue<Literal>().swap(propagation_queue);
            return true; // conflict
        }
    }
    return false; // no conflict
}

void SATSolver::set_number_of_variable(unsigned int n) {
    // right now, it is possible to set the number of variable only one time
    if ( number_of_variable != 0 )
        throw std::runtime_error("multiple resize not supported");

    number_of_variable = n;

    for ( size_t i = 0; i < n; ++i) {
        watch_list[Literal(i,true)];
        watch_list[Literal(i,false)];
    }

    values.resize(n,LIT_UNASIGNED);
    decision_levels.resize(n,-1);
    antecedents.resize(n,nullptr);
    conflict_clause = nullptr;
    vsids.set_size(n);
}

Literal SATSolver::decide_new_literal() {
    return vsids.select_new(values);
}

void SATSolver::build_unsat_proof() {

    for( auto it = trial.rbegin(); it != trial.rend(); ++it) {

        if (find(conflict_clause->begin(), conflict_clause->end(),
                    !(*it)) != conflict_clause->end()) {

            log.verbose << "\t resolve " << conflict_clause->print() << " and "
                << antecedents[it->atom()]->print();

            std::vector<Literal> new_lit;
            for ( const auto& l : *conflict_clause )
                if ( l != !(*it) ) new_lit.push_back(l);

            for ( auto l : *(antecedents[ it->atom() ]) ) {
                if (l == *it) continue;
                if (find(new_lit.begin(),new_lit.end(),l) != new_lit.end())
                    continue;
                new_lit.push_back(l);
            }
            log.verbose << " -> "  << new_lit << endl;

            auto new_ptr = make_shared<Clause>(*this,new_lit, true,
                    conflict_clause, antecedents[it->atom()]);
            conflict_clause = new_ptr;
            if ( conflict_clause->size() == 0 ) break; // found the empty clause
        }
    }
}

int SATSolver::conflict_analysis() {

    log.verbose << "learning new clausole from " <<
        conflict_clause->print() << endl;

    int count = 0;
    for ( const auto& l : *conflict_clause )
        if ( decision_levels[l.atom()] == current_level )
            ++count;

    // build the conflict clause
    for( auto it = trial.rbegin(); it != trial.rend() && count >= 2; ++it) {

        if ( decision_levels[it->atom()] < current_level ) break;

        if (find(conflict_clause->begin(), conflict_clause->end(),
                    !(*it)) != conflict_clause->end()) {
            // resolution process

            assert_message( antecedents[ it->atom() ] != nullptr,
                    "Unexpected decision literal");

            log.verbose << "\t" << conflict_clause->print() << " | "
                << antecedents[it->atom()]->print();
            count--;

            // increase activity of antecedent
            antecedents[ it->atom() ]->update_activity();

            std::vector<Literal> new_lit;
            for ( const auto& l : *conflict_clause )
                if ( l != !(*it) ) new_lit.push_back(l);

            for ( auto l : *(antecedents[ it->atom() ]) ) {
                if (l == *it) continue;
                if (find(new_lit.begin(),new_lit.end(),l) != new_lit.end())
                    continue;
                new_lit.push_back(l);
                if ( decision_levels[l.atom()] == current_level) count++;
            }
            log.verbose << " -> "  << new_lit << endl;
            auto new_ptr = make_shared<Clause>(*this,new_lit, true,
                    conflict_clause, antecedents[it->atom()]);

            conflict_clause = new_ptr;
        }
    }

    // find the backjump level, is the manimum level for witch the learned
    // clause is an assertion clause
    int backjump_level = 0;
    for ( const auto& i : *conflict_clause ) {
        if ( decision_levels[i.atom()] == current_level ) continue;
        backjump_level = max(backjump_level,decision_levels[i.atom()]);
    }

    return backjump_level;
}

bool SATSolver::learn_clause() {

    // add insert the new clause
    assert_message( conflict_clause != nullptr, "no conflict clause");
    learned.push_back(conflict_clause);
    learned.back()->initialize_structure();

    // initialize vsids info
    for ( const auto& l : *conflict_clause ) vsids.update(l);
    // increase activity of conflict clause
    conflict_clause->update_activity();

    // look for the assertion litteral
    for ( const auto& l : *conflict_clause ) {
        if ( get_asigned_value(l) == LIT_UNASIGNED ) {
            assign( l, learned.back());
            break;
        }
    }

    conflict_clause = nullptr; // reset conflict clause

    return false; // no conflict
}

bool SATSolver::add_clause( const std::vector<Literal>& c ) {
    vector<Literal> new_c;

    // remove repetition
    for ( const auto& l : c ) {
        if ( find(new_c.begin(),new_c.end(),l) == new_c.end())
            new_c.push_back(l);
    }

    // an empty clause is a conflict
    if ( new_c.size() == 0 ) return true; // conflict

    // add insert the new clause
    clauses.push_back(make_shared<SATSolver::Clause>(*this, new_c,false));
    clauses.back()->initialize_structure();

    // initialize vsids info
    for ( const auto& l : new_c ) vsids.update(l);

    // is unit
    if ( new_c.size() == 1 ) {
        bool conflict = assign( new_c[0], clauses.back());
        if ( conflict ) return true; // conflict
    }

    return false; // no conflict
}

void SATSolver::backtrack(int backtrack_level) {

    assert_message(backtrack_level <= current_level,
            "impossible to backtrack forward");

    log.verbose << "bactrack to level " << backtrack_level << " from " << current_level << endl;
    for ( int i = trial.size()-1;
            i >= 0 && decision_levels[trial[i].atom()] > backtrack_level; --i) {
        log.verbose << "\tunassign " << trial[i] << endl;

        values[trial[i].atom()] = LIT_UNASIGNED;
        decision_levels[trial[i].atom()] = -1;
        antecedents[trial[i].atom()] = nullptr;
        number_of_assigned_variable--;

        trial.pop_back();

    }
    current_level = backtrack_level;
}

bool SATSolver::subset( const Clause& inner,const Clause& outer) {
    // prefilter using signature
    if ( (inner.get_signature() & ~outer.get_signature()) != 0 ) return false;

    // more expansive but precise inclusion check
    for ( const auto& l : inner )
        if ( find(outer.begin(), outer.end(), l) == outer.end() )
            return false;

    return true;
}

void SATSolver::preprocessing() {
    log.normal << "preprocessing " << clauses.size() << " clauses\n";

    // collect clause to eliminate
    set<shared_ptr<Clause> > to_remove;

    for ( const auto& c : clauses ) {

        if ( to_remove.find(c) != to_remove.end() ) continue; // already removed

        // find the literal with the minimum neighbourn, it's the cheaper
        // to iterate.
        auto min_lit = min_element(c->begin(),c->end(),
                [&](const Literal& i, const Literal& j)
                {return subsumption[i].size() < subsumption[j].size();});

        // iterate all the clause that contain a compatible literal.
        for ( const auto& o : subsumption[*min_lit] ) {
            if ( o != c && c->size() <= o->size() && subset(*c,*o) ) {
                // if is possible to subsume a clause, put it inside the set
                log.verbose << "\tsubsume " << o->print() <<
                    " from " << c->print() << endl;
                to_remove.insert(o);
            }
        }
    }

    log.normal << "removed " << to_remove.size() << " clauses\n";

    // generate a new clause vector without the subsumed clauses
    vector<shared_ptr<Clause> > new_clauses;
    for ( const auto& c : clauses )
        if ( to_remove.find(c) == to_remove.end() )
            new_clauses.push_back(c);
    swap(clauses,new_clauses);

}

string SATSolver::string_conterproof() {
    ostringstream oss;
    // print the justification for the conflict clause
    conflict_clause->print_justification(oss);
    return oss.str();
}

void SATSolver::clause_activity_decay() {

    if ( clause_activity_update > 1e100 ) {
        for ( auto & c : learned )
            c->activity/=clause_activity_update;
        clause_activity_update = 1.0;
    }

    clause_activity_update*=clause_decay_factor;
}

void SATSolver::reduce_learned() {
    size_t i = 0, j = 0;
    // sort learned clause by activity (in ascending order)
    sort(learned.begin(), learned.end(),
            [&](const shared_ptr<Clause>& l, const shared_ptr<Clause>& r)
                { return l->activity < r->activity; });

    log.verbose << "sorted, from " << learned.front()->activity <<
        " to " << learned.back()->activity << " of " <<
        learned.size() << " clauses"<< endl;

    // delete the first half of the clause, exept clauses that are
    // the antecedent of some assigned literal
    set<shared_ptr<Clause> > not_removable;
    for ( const auto& c : antecedents ) 
        if ( c != nullptr && c->is_learned() )
            not_removable.insert(c);

    // remove the first half
    for ( ; i < learned.size()/2 ; ++i ) {
        if ( not_removable.find(learned[i]) != not_removable.end() )
            learned[j++] = learned[i];
        else
            learned[i]->remove();
    }

    // move the second half in the beginning
    for (; i < learned.size(); ++i)
        learned[j++] = learned[i];

    log.verbose << "after elimination, from " << learned.front()->activity <<
        " to " << learned.back()->activity << " of " <<
        learned.size() << " clauses"<< endl;

    // keep only the most active clause
    learned.resize( j );
}

void SATSolver::set_clause_decay(double cd_factor) {
    clause_decay_factor = cd_factor;
}

void SATSolver::set_literal_decay(double ld_factor) {
    vsids.set_parameter(ld_factor);
}

} // end namespace Satyricon

