#include <array>
#include <queue>
#include <algorithm>
#include <tuple>
#include <iomanip>
#include "assert_message.hpp"
#include "clause.hpp"
#include "log.hpp"
#include "sat_solver.hpp"
#include "vsids.hpp"

using std::endl; using std::setw;
using std::vector; using std::string; using std::queue; using std::set;

namespace Satyricon {

// defualt constructor
SATSolver::SATSolver():
    clauses(),
    learned(),
    number_of_variable(0),
    watch_list(),
    values(),
    decision_levels(),
    antecedents(),
    propagation_queue(),
    vsids(),
    trail(),
    trail_limit(),
    log(Utils::LOG_NONE),
    model(),
    subsumption(),
    enable_preprocessing(true),
    enable_restart(true),
    enable_deletion(true),
    restart_interval_multiplier(10),
    restart_threshold(1),
    clause_activity_update(1.0),
    clause_decay_factor(1.0 / 0.999),
    initial_learn_mult (2.0),
    percentual_learn_increase(50.0)
{}

SATSolver::~SATSolver() {
    log.verbose << "destroy original clauses" << endl;
    for ( auto &c : clauses )
        remove_clause(c);
    log.verbose << "destroy learned clauses, size: " << learned.size() << endl;
    for ( auto &c : learned ) {
        log.verbose << "remove " << c->print() << endl;
        remove_clause(c);
    }
}

bool SATSolver::solve() {
    // main method
    log.normal << "begin solve" << endl;

    // initialize search parameter
    unsigned int conflict_counter = 0;
    unsigned int restart_counter = 0;
    unsigned int learn_limit = static_cast<unsigned int>(
            static_cast<double>(clauses.size())*initial_learn_mult );
    restart_threshold = new_restart_threshold();

    // preprocess
    if ( enable_preprocessing) { preprocessing(); }

    while ( true ) { // loop until a solution is found

        log.verbose << "propagate at level " << current_level() << endl;
        // propagate assingment effect
        ClausePtr conflict = propagation();

        if ( conflict != nullptr ) {

            conflict_counter++;
            // print update info every 1000s conflict
            if ( conflict_counter % 1000 == 0 )
                print_status(conflict_counter,restart_counter, learn_limit);

            // if a conflict is found on level 0, it is impossible to solve
            // so the formula must be unsatisfiable
            if ( current_level() == 0 ) {
                log.verbose << "conflict at level 0, build unsat proof\n";
                print_status(conflict_counter,restart_counter, learn_limit);
                return false; // UNSAT
            }

            // otherwise, analize the conflict and backtrack

            int backtrack_level;
            vector<Literal> conflict_literals;
            conflict_analysis(conflict, conflict_literals,backtrack_level);

            cancel_until( backtrack_level );
            learn_clause(conflict_literals); // learn the conflcit clause

            // after a conflict, the activity of literals and clauses decay
            vsids.decay();
            clause_activity_decay();
        }
        else {
            // no conflict and no more value to propagate

            // if all variables are asigned, the problem is satisfiable
            if ( number_of_assigned_variable() == number_of_variable ) {
                log.verbose << "assinged all literals without conflict\n";
                build_sat_proof();
                print_status(conflict_counter,restart_counter, learn_limit);
                return true; // SAT
            }

            // if the learning limit is reached, the learned clause must
            // be reduced, the new learning limit is now higher
            if ( enable_deletion && learned.size() >= learn_limit ) {
                /*
                // cast for suppres warning
                learn_limit += static_cast<unsigned int>(
                        (learn_limit*percentual_learn_increase)/100.0);

                reduce_learned();
                */
            }

            if ( enable_restart && conflict_counter >= restart_threshold ) {
                // if the restart limit is reached, bactrack to level zero
                // and select the new threshold for the restart process
                restart_counter++;
                restart_threshold += new_restart_threshold();
                log.verbose << "restarting." <<
                    "next restart at " << restart_threshold << endl;
                cancel_until(0);
            }
            else {
                // otherwise open a new decision level and decide a new literal
                // based on the vsids heuristic
                Literal l = vsids.select_new(values);
                log.verbose << "decide literal " << l << endl;
                assume(l);
            }
        }
    }
}


bool SATSolver::assume( Literal p ) {
    assert_message( get_asigned_value(p) == LIT_UNASIGNED,
            "deciding an already assigned literal");
    trail_limit.push_back(static_cast<int>(trail.size()));
    return assign(p, nullptr);
}

void SATSolver::cancel() {
    int i = static_cast<int>(trail.size()) - trail_limit.back();
    for (; i != 0; i--)
        undo_one();
    trail_limit.pop_back();
}

inline int SATSolver::current_level() const {
    return static_cast<int>(trail_limit.size());
}

void SATSolver::cancel_until( int level ) {
    log.verbose << "backtrack from " << current_level() <<
        " to " << level << endl;
    while ( current_level() > level )
        cancel();
}

void SATSolver::print_status(unsigned int conflict, unsigned int restart,
        unsigned int learn_limit) {
    log.normal << "conflict: " << setw(7) << conflict;
    if ( enable_restart )
        log.normal << ", restart: " << setw(7) <<  restart;
    if ( enable_deletion )
        log.normal << ", learn limit: " << setw(7) <<  learn_limit;
    log.normal << ", learned: " << setw(7) <<  learned.size() << endl;
}

unsigned int SATSolver::next_restart_interval() {
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

unsigned int SATSolver::new_restart_threshold() {
    return restart_interval_multiplier * next_restart_interval();
}

void SATSolver::build_sat_proof() {
    int val = 1;
    model.clear();
    // map the assigned value to an int rappresentation in DIMACS format
    transform(values.begin(), values.end(), back_inserter(model),
            [&val](literal_value v) { return v == LIT_TRUE ? val++ : -val++; });
}

const vector<int>& SATSolver::get_model() {
    return model;
}

string SATSolver::string_model() {
    std::ostringstream oss;
    oss << "[ ";
    for ( auto i : model ) oss << i << " ";
    oss << "]";
    return oss.str();
}

literal_value SATSolver::get_asigned_value(const Literal & l) const {
    // LIT_TRUE = 2, LIT_UNASIGNED = 1, LIT_FALSE = 2
    // so for the negated result we can do 2 - value
    if ( l.sign() )
        return static_cast<literal_value>(LIT_TRUE - values[l.var()]);
    return values[l.var()];
}

bool SATSolver::assign(Literal l, ClausePtr antecedent) {
    // already assigned ?
    if ( get_asigned_value(l) == LIT_TRUE )
        return false; // already assigned, no conflict
    if ( get_asigned_value(l) == LIT_FALSE )
        return true; // conflict!

    log.verbose << "\tassign literal " << l <<
        ",  level " << current_level() << ", antecedent " <<
        (antecedent == nullptr ? "NONE" : antecedent->print() ) << endl;

    // unassigned, update asignment
    values[l.var()] = l.sign() ? LIT_FALSE : LIT_TRUE;
    decision_levels[l.var()] = current_level();
    antecedents[l.var()] = antecedent;

    // save the current decision, for eventual backtrack
    trail.push_back(l);

    // the assignment effect must propagate
    propagation_queue.push(l);

    return false; // no conflict found
}

size_t SATSolver::number_of_assigned_variable() const {
    return trail.size();
}

SATSolver::ClausePtr SATSolver::propagation() {
    while ( ! propagation_queue.empty() ) {
        // fetch an element from the queue
        auto l = propagation_queue.front(); propagation_queue.pop();
        log.verbose << "propagate " << l << endl;

        // extract the list of the opposite literal 
        // (they are false now, their watcher must be moved)
        auto failed = !l;
        vector<ClausePtr> to_move;
        swap(to_move,watch_list[failed.index()]);

        for (auto it = to_move.begin(); it != to_move.end(); ++it) {

            // propagate on a clause
            bool conflict = (*it)->propagate(*this,failed);
            if ( ! conflict ) continue; // no problem, move to the next

            // conflict found in propagation 
            log.verbose << "\tfound a conflict on " << (*it)->print() << endl;
            ClausePtr conflict_clause = *it;

            // reset the other literal to move (it is already set by propagate)
            copy(++it, to_move.end(), back_inserter(watch_list[failed.index()]));

            // clear the propagation
            queue<Literal>().swap(propagation_queue);
            return conflict_clause;
        }
    }
    return nullptr; // no conflict
}

void SATSolver::conflict_analysis(ClausePtr conflict, vector<Literal> &out_learnt, int &out_btlevel) {
    assert_message(out_learnt.empty(), "out_learnt must be empty");
    vector<bool> seen(number_of_variable, false);
    int counter = 0;
    Literal p = UNDEF_LIT;
    vector<Literal> p_reason;

    out_btlevel = 0;
    // free space for assertion literal
    out_learnt.push_back(Literal());

    do {
        p_reason.clear();
        conflict->calcReason(*this, p, p_reason);

        // trace reason of P
        for ( const auto &q : p_reason ) {
            if ( ! seen[q.var()] ) {
                seen[q.var()] = true;
                if ( decision_levels[q.var()] == current_level() )
                    ++counter;
                else if ( decision_levels[q.var()] > 0 ) {
                    out_learnt.push_back(!q);
                    out_btlevel = std::max(out_btlevel, decision_levels[q.var()]);
                }
            }
        }
        do {
            p = trail.back();
            conflict = antecedents[p.var()];
            undo_one();
        } while ( ! seen[p.var()] );
        --counter;
    } while ( counter > 0 );
    out_learnt[0] = !p;
}

void SATSolver::undo_one() {
    Literal p = trail.back();
    values[p.var()] = LIT_UNASIGNED;
    antecedents[p.var()] = nullptr;
    decision_levels[p.var()]  = -1;
    trail.pop_back();
}

bool SATSolver::new_clause(vector<Literal> &c, bool learnt, ClausePtr &c_ref) {

    c_ref = nullptr;

    if ( ! learnt ) { // simplify if possible, learned clause doesn't need this
        size_t j = 0;
        for ( size_t i = 0; i < c.size(); i++ ) {
            // discard false literal
            if ( get_asigned_value( c[i] ) == LIT_FALSE ) continue;
            // already satisfied?
            if ( get_asigned_value( c[i] ) == LIT_TRUE  )
                return false; // no conflict, new clause is a nullptr
            // look for tautology or repetition
            bool add = true;
            for ( size_t k = i+1; k < c.size(); ++k ) {
                if ( c[k] == c[i] ) { // repetition?
                    add = false;      // don't add the repeated literal
                    break;
                }
                if ( c[k] == !(c[i]) ) // tautology?
                    return false; // no conflict, new clause is a nullptr 
            }
            if ( add ) c[j++] = c[i];
        }
        c.resize(j);
    }

    // an empty clause is a conflict
    if (c.empty()) return true; // conflict, cnew clause is a nullptr

    // a clause with 1 literal is a unit, simply assign it
    if ( c.size() == 1 ) {
        // true if the assignment is in conflict, false otherwise
        // the new clause is a nullptr (don't build clause of one literal)
        return  assign( c[0], nullptr );
    }

    // build the clause
    c_ref = Clause::allocate(c,learnt);

    // initialize vsids info
    for ( const auto& l : c ) vsids.update(l);


    if ( learnt ) { 
        // pick a correct second literal to watch
        auto second = c_ref->begin()+1;
        for ( auto it = c_ref->begin()+2; it != c_ref->end(); ++it)
            if ( decision_levels[it->var()] > decision_levels[second->var()] )
                second = it;
        // swap
        Literal tmp = *second;
        *second = c_ref->at(1);
        c_ref->at(1) = tmp;

        // increase activity
        c_ref->update_activity( *this );
    }

    //  add to the watch list
    watch_list[c_ref->at(0).index()].push_back(c_ref);
    watch_list[c_ref->at(1).index()].push_back(c_ref);

    if ( ! learnt ) { // put every litteral in the subsumption queue
        for ( const auto& l : c )
            subsumption[l.index()].push_back(c_ref);
    }

    return false; // no conflict
}

bool SATSolver::add_clause(std::vector<Literal>& lits) {
    // build the new clause
    ClausePtr clause;
    bool conflict = new_clause(lits, false, clause);
    // if the clause is a conflict, return immediatly
    if ( conflict ) return true; // conflict
    // clause is nullptr if the new clause is a unit
    if ( clause != nullptr ) clauses.push_back(clause);
    return false; // no conflict
}

void SATSolver::learn_clause(std::vector<Literal> & lits) {
    log.verbose << "learn clause " << lits << endl;
    // build the new clause, it's never a conflict if the clause is learned
    ClausePtr clause;
    new_clause(lits, true, clause);
    // the learned clause is always a unit, with the unasigned literal in 0
    assign(lits[0],clause);
    log.verbose << "address " << clause << std::endl;
    // if the clause have only one literal, don't add that to the list
    if ( clause != nullptr ) learned.push_back(clause);
}

void SATSolver::remove_from_vect( std::vector<ClausePtr> &v, ClausePtr c ) {
    for ( auto &i : v ) {
        if ( i == c ) {
            i = v.back();
            v.pop_back();
            return;
        }
    }
    assert_message(false, "try to remove a nonexistent object");
}

void SATSolver::remove_clause( ClausePtr c ) {
    remove_from_vect( watch_list[c->at(0).index()], c );
    remove_from_vect( watch_list[c->at(1).index()], c );

    if ( !c->is_learned() ) {
        for ( const auto &i : *c )
            remove_from_vect( subsumption[i.index()], c );
    }

    Clause::deallocate( c );
}

bool SATSolver::subset( const Clause& inner,const Clause& outer) {
    // prefilter using signature
    if ( (inner.get_signature() & ~outer.get_signature()) != 0 ) return false;

    // more expansive but precise inclusion check
    for ( const auto& l : inner )
        if ( std::find(outer.begin(), outer.end(), l) == outer.end() )
            return false;

    return true;
}

void SATSolver::preprocessing() {
    /*
    log.normal << "preprocessing " << clauses.size() << " clauses\n";

    // collect clause to eliminate
    set<ClausePtr > to_remove;

    for ( const auto& c : clauses ) {

        if ( to_remove.find(c) != to_remove.end() ) continue; // already removed

        // find the literal with the minimum number of neighbourns, it's the
        // cheaper to iterate.
        auto min_lit = std::min_element(c->begin(),c->end(),
                [&](const Literal& i, const Literal& j)
                {return subsumption[i.index()].size() < subsumption[j.index()].size();});

        // iterate all the clause that contain a compatible literal.
        for ( const auto& o : subsumption[min_lit->index()] ) {
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
    vector<ClausePtr > new_clauses;
    for ( const auto& c : clauses )
        if ( to_remove.find(c) == to_remove.end() )
            new_clauses.push_back(c);
    swap(clauses,new_clauses);
    */
}

void SATSolver::clause_activity_decay() {

    // if big value is reached, a normalization is required
    if ( clause_activity_update > 1e100 ) {
        for ( auto & c : learned )
            c->renormalize_activity(*this);
        clause_activity_update = 1.0;
    }

    clause_activity_update*=clause_decay_factor;
}

void SATSolver::reduce_learned() {
    /*
    size_t i = 0, j = 0; // use this indices to compact the vector
    // sort learned clause by activity (in ascending order)
    sort(learned.begin(), learned.end(),
            [&](const ClausePtr& l, const ClausePtr& r)
                { return l->get_activity() < r->get_activity(); });

    // delete the first half of the clauses, exept clauses that are
    // the antecedent of some assigned literal
    set<ClausePtr > not_removable;
    for ( const auto& c : antecedents ) 
        if ( c != nullptr && c->is_learned() )
            not_removable.insert(c);

    // remove the first half
    for ( ; i < learned.size()/2 ; ++i ) {
        if ( not_removable.find(learned[i]) != not_removable.end() )
            learned[j++] = learned[i]; // keep the justification
        else
            Clause::deallocate(learned[i]); // TODO: ricontrolla
    }

    // move the second half in the first half (this effectively delete the
    // low activity clauses)
    for (; i < learned.size(); ++i) learned[j++] = learned[i];

    // keep only the most active clause
    learned.resize( j );
    */
}

void SATSolver::set_number_of_variable(unsigned int n) {
    // right now, it is possible to set the number of variable only one time
    if ( number_of_variable != 0 )
        throw std::runtime_error("multiple resize not supported yet");

    number_of_variable = n;

    watch_list.resize( 2 * number_of_variable );
    subsumption.resize( 2 * number_of_variable );

    values.resize(n,LIT_UNASIGNED);
    decision_levels.resize(n,-1);
    antecedents.resize(n,nullptr);
    vsids.set_size(n);
}

void SATSolver::set_log( Utils::Log l) {
    log = l;
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

void SATSolver::set_clause_decay(double decay) {
    assert_message( decay > 0.0 && decay <= 1.0, "must be 0.0 < decay ≤ 0.1 ");
    clause_decay_factor = 1.0 / decay;
}

void SATSolver::set_literal_decay(double ld_factor) {
    vsids.set_parameter(ld_factor);
}

void SATSolver::set_learning_multiplier( double value ) {
    initial_learn_mult = value;
}

void SATSolver::set_learning_increase( double value ) {
    percentual_learn_increase = value;
}

} // end namespace Satyricon

