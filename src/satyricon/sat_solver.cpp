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

// defualt constructor
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
    clause_decay_factor(1.0 / 0.999),
    initial_learn_mult (2.0),
    percentual_learn_increase(50.0)
{}

bool SATSolver::solve() {
    // main method
    log.normal << "begin solve" << endl;

    // initialize search parameter
    unsigned int conflict_counter = 0;
    int restart_counter = 0;
    unsigned int learn_limit = clauses.size()*initial_learn_mult;
    restart_threshold = new_restart_threshold();

    // preprocess
    if ( enable_preprocessing) { preprocessing(); }

    while ( true ) { // loop until a solution is found

        log.verbose << "propagate at level " << current_level << endl;
        // propagate assingment effect
        bool conflict = propagation();

        if ( conflict ) {

            conflict_counter++;
            // print update info every 1000s conflict
            if ( conflict_counter % 1000 == 0 )
                print_status(conflict_counter,restart_counter, learn_limit);

            // if a conflict is found on level 0, it is impossible to solve
            // so the formula must be unsatisfiable
            if ( current_level == 0 ) {
                log.verbose << "conflict at level 0, build unsat proof\n";
                build_unsat_proof();
                print_status(conflict_counter,restart_counter, learn_limit);
                return false; // UNSAT
            }

            // otherwise, analize the conflict and backtrack

            auto backtrack_level  = conflict_analysis();

            backtrack( backtrack_level );
            learn_clause(); // learn the conflcit clause

            // after a conflict, the activity of literals and clauses decay
            vsids.decay();
            clause_activity_decay();
        }
        else {
            // no conflict and no more value to propagate

            // if all variables are asigned, the problem is satisfiable
            if ( number_of_assigned_variable == number_of_variable ) {
                log.verbose << "assinged all literals without conflict\n";
                build_sat_proof();
                print_status(conflict_counter,restart_counter, learn_limit);
                return true; // SAT
            }

            // if the learning limit is reached, the learned clause must
            // be reduced, the new learning limit is now higher
            if ( enable_deletion && learned.size() >= learn_limit ) {
                learn_limit += (learn_limit*percentual_learn_increase)/100.0;
                reduce_learned();
            }

            if ( enable_restart && conflict_counter >= restart_threshold ) {
                // if the restart limit is reached, bactrack to level zero
                // and select the new threshold for the restart process
                restart_counter++;
                restart_threshold += new_restart_threshold();
                log.verbose << "restarting." <<
                    "next restart at " << restart_threshold << endl;
                backtrack(0);
            }
            else {
                // otherwise open a new decision level and decide a new literal
                // based on the vsids heuristic
                current_level++;
                Literal l = vsids.select_new(values);
                log.verbose << "decide literal " << l << endl;
                assign( l, nullptr ); // assign the decision
            }
        }
    }
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
    ostringstream oss;
    oss << "[ ";
    for ( auto i : model ) oss << i << " ";
    oss << "]";
    return oss.str();
}

literal_value SATSolver::get_asigned_value(const Literal & l) {
    // LIT_TRUE = 2, LIT_UNASIGNED = 1, LIT_FALSE = 2
    // so for the negated result we can do 2 - value
    if ( l.is_negated() )
        return (literal_value)(LIT_TRUE - values[l.atom()]);
    else
        return values[l.atom()];
}

bool SATSolver::assign(Literal l, ClausePtr antecedent) {
    // already assigned ?
    if ( get_asigned_value(l) == LIT_TRUE )
        return false; // already assigned, no conflict
    if ( get_asigned_value(l) == LIT_FALSE )
        return true; // conflict!

    log.verbose << "\tassign literal " << l <<
        ",  level " << current_level << ", antecedent " <<
        (antecedent == nullptr ? "NONE" : antecedent->print() ) << endl;

    // unassigned, update asignment
    ++number_of_assigned_variable;
    values[l.atom()] = l.is_negated() ? LIT_FALSE : LIT_TRUE;
    decision_levels[l.atom()] = current_level;
    antecedents[l.atom()] = antecedent;

    // save the current decision, for eventual backtrack
    trial.push_back(l);

    // the assignment effect must propagate
    propagation_queue.push(l);

    return false; // no conflict found
}

bool SATSolver::propagation() {
    while ( ! propagation_queue.empty() ) {
        // fetch an element from the queue
        auto l = propagation_queue.front(); propagation_queue.pop();
        log.verbose << "propagate " << l << endl;

        // extract the list of the opposite literal 
        // (they are false now, their watcher must be moved)
        auto failed = !l;
        list<ClausePtr> to_move;
        swap(to_move,watch_list[failed]);

        for (auto it = to_move.begin(); it != to_move.end(); ++it) {

            // propagate on a clause
            bool conflict = (*it)->propagate(failed);
            if ( ! conflict ) continue; // no problem, move to the next

            // conflict found in propagation 
            log.verbose << "\tfound a conflict on " << (*it)->print() << endl;

            // initialize the conflict clause
            conflict_clause = (*it);

            // reset the other literal to move (it is already set by propagate)
            copy(++it, to_move.end(), back_inserter(watch_list[failed]));

            // clear the propagation
            queue<Literal>().swap(propagation_queue);
            return true; // conflict
        }
    }
    return false; // no conflict
}

int SATSolver::conflict_analysis() {
    log.verbose << "learning new clausole from " <<
        conflict_clause->print() << endl;

    // count how many variables are assigned on the current level.
    int this_level = 0;
    for ( const auto& l : *conflict_clause )
        if ( decision_levels[l.atom()] == current_level )
            ++this_level;

    conflict_clause->update_activity();

    // build the conflict clause
    for(auto it = trial.rbegin(); it != trial.rend() && this_level >= 2; ++it) {

        // search for the literal with inverse polarity, skip if missing
        if (find(conflict_clause->begin(), conflict_clause->end(),
                    !(*it)) == conflict_clause->end())
            continue; // nothing to resolve

        // resolution process
        log.verbose << "\tresolve " << conflict_clause->print() <<
            " and " << antecedents[it->atom()]->print();

        // decrease the counter, a literal of the current level is removed
        this_level--;

        // increase activity of antecedent
        antecedents[ it->atom() ]->update_activity();

        std::vector<Literal> new_lit; // literals of the new clause
        // put everything except the resolved literal
        for ( const auto& l : *conflict_clause )
            if ( l != !(*it) ) new_lit.push_back(l);

        for ( auto l : *(antecedents[ it->atom() ]) ) {
            if (l == *it) continue; // no resolved literal
            if (find(new_lit.begin(),new_lit.end(),l) != new_lit.end())
                continue; // no duplicates
            // add all the other
            new_lit.push_back(l);
            // if this is also decided at this level, increase the counter
            if ( decision_levels[l.atom()] == current_level) this_level++;
        }

        // sort by decreasing level, so the assertions literals are in
        // the beginning. It's usefull also for the backjump
        sort(new_lit.begin(),new_lit.end(),
                [&](const Literal& l, const Literal& r) {
                    return decision_levels[l.atom()] > decision_levels[r.atom()];});

        log.verbose << " -> "  << new_lit << endl;
        // build the learned clause
        auto new_ptr = make_shared<Clause>(*this,new_lit, true,
                conflict_clause, antecedents[it->atom()]);

        conflict_clause = new_ptr;
    }

    // the conflict clause is an assertion clause, with the literal ordered by
    // level. So the litteral in position 0 is in the conflict level, and the
    // second one have the level of the backjump
    if (conflict_clause->size() > 1)
        return decision_levels[conflict_clause->at(1).atom()];
    else
        return 0;
}

void SATSolver::build_unsat_proof() {
    // this process is really similar to the analysis of conflict, but the
    // process stop when only when the empty clause is reached
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

bool SATSolver::learn_clause() {

    // learn the conflict clause
    assert_message( conflict_clause != nullptr, "no conflict clause");
    learned.push_back(conflict_clause);
    learned.back()->initialize_structure();

    // increase activity for all the literals inside the new clause
    for ( const auto& l : *conflict_clause ) vsids.update(l);

    // increase activity of conflict clause
    conflict_clause->update_activity();

    // search the assertion literal (the only unsasigned literal after backjump)
    for ( const auto& l : *conflict_clause ) {
        if ( get_asigned_value(l) == LIT_UNASIGNED ) {
            // assign the assertion literal, the conflict clause is a unit
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

    // add the new clause to the problem
    clauses.push_back(make_shared<SATSolver::Clause>(*this, new_c,false));
    clauses.back()->initialize_structure();

    // initialize vsids info
    for ( const auto& l : new_c ) vsids.update(l);

    if ( new_c.size() == 1 ) {
        // is unit, assign the value immediately, it can result in a conflict
        // if two opposite literals appears in the original formula
        bool conflict = assign( new_c[0], clauses.back());
        if ( conflict ) return true; // conflict
    }

    return false; // no conflict
}

void SATSolver::backtrack(int backtrack_level) {
    assert_message(backtrack_level <= current_level,
            "impossible to backtrack forward");
    log.verbose << "bactrack to level " << backtrack_level <<
        " from " << current_level << endl;

    for ( int i = trial.size()-1; i >= 0; --i) {
        // end if the required level is reached
        if (decision_levels[trial[i].atom()] <= backtrack_level) break;

        // otherwise keep unasigning the assigned value
        log.verbose << "\tunassign " << trial[i] << endl;
        values[trial[i].atom()] = LIT_UNASIGNED;
        decision_levels[trial[i].atom()] = -1;
        antecedents[trial[i].atom()] = nullptr;
        number_of_assigned_variable--;

        // remove from the trial
        trial.pop_back();
    }
    current_level = backtrack_level; // reset the current level
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
    set<ClausePtr > to_remove;

    for ( const auto& c : clauses ) {

        if ( to_remove.find(c) != to_remove.end() ) continue; // already removed

        // find the literal with the minimum number of neighbourns, it's the
        // cheaper to iterate.
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
    vector<ClausePtr > new_clauses;
    for ( const auto& c : clauses )
        if ( to_remove.find(c) == to_remove.end() )
            new_clauses.push_back(c);
    swap(clauses,new_clauses);
}

string SATSolver::unsat_proof() {
    ostringstream oss;
    // the unsat proof is the the justification for the empty clause
    conflict_clause->print_justification(oss);
    return oss.str();
}

void SATSolver::clause_activity_decay() {

    // if big value is reached, a normalization is required
    if ( clause_activity_update > 1e100 ) {
        for ( auto & c : learned )
            c->activity/=clause_activity_update;
        clause_activity_update = 1.0;
    }

    clause_activity_update*=clause_decay_factor;
}

void SATSolver::reduce_learned() {
    size_t i = 0, j = 0; // use this indices to compact the vector
    // sort learned clause by activity (in ascending order)
    sort(learned.begin(), learned.end(),
            [&](const ClausePtr& l, const ClausePtr& r)
                { return l->activity < r->activity; });

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
            learned[i]->remove(); // remove from watch list
    }

    // move the second half in the first half (this effectively delete the
    // low activity clauses)
    for (; i < learned.size(); ++i) learned[j++] = learned[i];

    // keep only the most active clause
    learned.resize( j );
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

