#include <array>
#include <algorithm>
#include <tuple>
#include <iomanip>
#include "assert_message.hpp"
#include "sat_solver.hpp"

using std::endl; using std::setw; using std::max;
using std::vector; using std::string; using std::set;

namespace Satyricon {

/*
 * utility printing macro
 */
#ifndef NDEBUG
#define PRINT_VERBOSE(X)          \
if ( log_level >= 2 ) {           \
    std::cout << X;  /* NOLINT */ \
}
#else
#define PRINT_VERBOSE(X) {}
#endif

#define PRINT(X)                   \
if ( log_level >= 1 ) {            \
    std::cout << X;   /* NOLINT */ \
}

// defualt constructor
SATSolver::SATSolver():
    clauses(),
    learned(),
    number_of_variable(0),
    watch_list(),
    values(),
    decision_levels(),
    antecedents(),
    propagation_starting_pos(0),
    trail(),
    trail_limit(),
    log_level(1),
    model(),
    solve_conflict_literals(),
    propagation_to_move(),
    analisys_seen(),
    analisys_reason(),
    literals_activity(),
    order(literals_activity,values),
    seed_1(123456789),
    seed_2(362436000),
    seed_3(521288629),
    seed_4(7654321),
    param()
{}

SATSolver::~SATSolver() {
    for ( auto &c : clauses ) remove_clause(c);
    for ( auto &c : learned ) remove_clause(c);
}

bool SATSolver::solve() {
    // main method
    PRINT("begin solve" << endl);

    // initialize search parameter
    unsigned int conflict_counter = 0;
    unsigned int restart_counter = 0;
    unsigned int learn_limit = static_cast<unsigned int>(
            static_cast<double>(clauses.size())*param.initial_learn_mult );
    param.restart_threshold = new_restart_threshold();

    unsigned int increase_limit_threshold = 100;
    unsigned int increase_limit_counter = increase_limit_threshold;

    // order literal for decision
    order.initialize_heap();

    // preprocess
    if ( param.enable_preprocessing) { preprocessing(); }

    print_status(conflict_counter,restart_counter, learn_limit);
    while ( true ) { // loop until a solution is found

        PRINT_VERBOSE("propagate at level " << current_level() << endl);
        // propagate assingment effect
        ClausePtr conflict = propagation();

        if ( conflict != nullptr ) {

            conflict_counter++;

            // if a conflict is found on level 0, it is impossible to solve
            // so the formula must be unsatisfiable
            if ( current_level() == 0 ) {
                PRINT_VERBOSE("conflict at level 0, build unsat proof" << endl);
                print_status(conflict_counter,restart_counter, learn_limit);
                return false; // UNSAT
            }

            if ( --increase_limit_counter == 0 ) {
                increase_limit_threshold *= 1.5;
                increase_limit_counter = increase_limit_threshold;
                learn_limit += static_cast<unsigned int>(
                        (learn_limit*param.percentual_learn_increase)/100.0);

                print_status(conflict_counter,restart_counter, learn_limit);
            }
            // otherwise, analize the conflict and backtrack

            int backtrack_level;
            solve_conflict_literals.clear();
            conflict_analysis(conflict,solve_conflict_literals,backtrack_level);

            cancel_until( backtrack_level );
            learn_clause(solve_conflict_literals); // learn the conflcit clause

            // after a conflict, the activity of literals and clauses decay
            literals_activity_decay();
            clause_activity_decay();
        }
        else {
            // no conflict and no more value to propagate

            // if all variables are asigned, the problem is satisfiable
            if ( number_of_assigned_variable() == number_of_variable ) {
                PRINT_VERBOSE("assinged all literals without conflict" << endl);
                build_sat_proof();
                print_status(conflict_counter,restart_counter, learn_limit);
                return true; // SAT
            }

            if ( current_level() == 0 ) {
                //simplify(clauses);
                //simplify(learned);
            }

            // if the learning limit is reached, the learned clause must
            // be reduced, the new learning limit is now higher
            if ( param.enable_deletion && learned.size() >= learn_limit ) {
                // cast for suppres warning
                reduce_learned();
            }

            if ( param.enable_restart &&
                    conflict_counter >= param.restart_threshold ) {
                // if the restart limit is reached, bactrack to level zero
                // and select the new threshold for the restart process
                restart_counter++;
                param.restart_threshold += new_restart_threshold();
                PRINT_VERBOSE("restarting. next restart at "<<
                        param.restart_threshold<< endl);
                cancel_until(0);
            }

            // open a new decision level and decide a new literal
            // based on the vsids heuristic
            Literal l = choice_lit();
            PRINT_VERBOSE("decide literal " << l << endl);
            assume(l);
        }
    }
}

Literal SATSolver::choice_lit() {

    // random choice 1% of times
    if ( param.enable_random_choice &&  random() % 100 == 0 ) {
        int val;
        do {
            val = random() % number_of_variable;
        } while ( values[val] != LIT_UNASIGNED );
        return Literal ( val,static_cast<bool>(random() % 2) ) ;
    }

    // otherwise select from order
    return order.decision();
}

void SATSolver::simplify( vector<ClausePtr> &vect) {
    assert_message(current_level() == 0,"only at top level" );
    assert_message(propagation_starting_pos == trail.size(),
            "no semplification before propagation");
    size_t j = 0;
    for ( auto & c : vect ) {
        if ( simplify_clause(c) )
            remove_clause(c);
        else
            vect[j++] = c;
    }
    PRINT_VERBOSE("eliminated " << (vect.size()-j) << " clauses" << endl);
    vect.resize(j);
}

bool SATSolver::simplify_clause( ClausePtr c ) {
    size_t j = 0;
    for ( const auto & l : *c ) {
        if ( get_asigned_value(l) == LIT_TRUE )
            return true; // useless
        if ( get_asigned_value(l) == LIT_UNASIGNED )
            c->at(j++) = l;
    }
    c->shrink(j);
    return false; // still usefull
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
    PRINT_VERBOSE("backtrack from " << current_level() <<
            " to " << level << endl);
    while ( current_level() > level )
        cancel();
    propagation_starting_pos = trail.size();
}

void SATSolver::print_status(unsigned int conflict, unsigned int restart,
        unsigned int learn_limit) {
    PRINT("conflict: " << setw(7) << conflict);
    if ( param.enable_restart )
        PRINT(", restart: " << setw(7) <<  restart);
    if ( param.enable_deletion )
        PRINT(", learn limit: " << setw(7) <<  learn_limit);
    PRINT(", learned: " << setw(7) <<  learned.size() << endl);
}

unsigned int SATSolver::next_restart_interval( unsigned int pos) {
    // use the luby sequence for evaluating the new interval
    // the code is based on minisat
    unsigned int size, seq;

    // find in what sequence we are, the sequence i have lenght (2^i)-1
    for ( size = 1, seq = 0; size < pos+1; ++seq, size = 2*size + 1 )
        ;

    // find the value inside the sequence
    while ( (size-1) != pos ) {
        size /= 2;
        --seq;
        pos %= size;
    }

    return 1 << seq; // 2^seq
}

unsigned int SATSolver::new_restart_threshold() {
    return param.restart_interval_multiplier*next_restart_interval(luby_next++);
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

    PRINT_VERBOSE("\tassign literal " << l <<
        ",  level " << current_level() << ", antecedent " <<
        (antecedent == nullptr ? "NONE" : antecedent->print() ) << endl);

    // unassigned, update asignment
    values[l.var()] = l.sign() ? LIT_FALSE : LIT_TRUE;
    decision_levels[l.var()] = current_level();
    antecedents[l.var()] = antecedent;

    // save the current decision, for eventual backtrack
    trail.push_back(l);

    return false; // no conflict found
}

size_t SATSolver::number_of_assigned_variable() const {
    return trail.size();
}

SATSolver::ClausePtr SATSolver::propagation() {

    while ( propagation_starting_pos < trail.size() ) {

        PRINT_VERBOSE("propagate " << trail[propagation_starting_pos] << endl);

        // extract the list of the opposite literal 
        // (they are false now, their watcher must be moved)
        auto failed = !trail[propagation_starting_pos++];
        propagation_to_move.clear();
        swap(propagation_to_move,watch_list[failed.index()]);

        for (auto it = propagation_to_move.begin();
                it != propagation_to_move.end(); ++it) {

            // propagate effect on a clause
            Clause &c = *(it->get_clause()); // usefull reference
            assert_message(c[0]==failed || c[1]==failed,"moving a non watched");

            // make sure the false literal is in position 1
            if ( c[0] == failed ) { c[0] = c[1]; c[1] = failed; }

            // if the clause is already solved, nothing need to be moved
            if (get_asigned_value(c[0]) == LIT_TRUE) {
                // reinsert inside the previous watch list
                watch_list[failed.index()].push_back(*it);
                continue; // move to the next
            }

            // search a new literal to watch
            bool foundt_new_watch = false;
            for ( size_t pos = 2; pos != c.size(); ++pos) {
                if ( get_asigned_value(c[pos]) != LIT_FALSE ) {
                    // swap value
                    c[1] = c[pos]; c[pos] = failed;
                    // insert in the new watch list
                    watch_list[c[1].index()].push_back(*it);
                    // move to the next
                    foundt_new_watch = true;
                    break;
                }
            }
            if ( foundt_new_watch ) continue; // move to the next

            // no new literal to watch, reinsert in the old position
            watch_list[failed.index()].push_back(*it);

            // the clause must be a conflict or a unit, try to assign the value
            bool conflict = assign(c[0],it->get_clause());
            if ( ! conflict ) continue; // no problem, move to the next

            // conflict found in propagation 
            PRINT_VERBOSE("\tfound a conflict on " << (it->get_clause())->print() << endl);
            ClausePtr conflict_clause = it->get_clause();

            // reset the other literal to move (it is already set by propagate)
            copy(++it, propagation_to_move.end(),
                    back_inserter(watch_list[failed.index()]));

            return conflict_clause;
        }
    }
    return nullptr; // no conflict
}

void SATSolver::conflict_analysis(ClausePtr conflict,
        vector<Literal> &out_learnt, int &out_btlevel) {
    assert_message(out_learnt.empty(), "out_learnt must be empty");
    std::fill(analisys_seen.begin(), analisys_seen.end(),false);
    int counter = 0;
    Literal p = UNDEF_LIT;

    out_btlevel = 0;
    // free space for assertion literal
    out_learnt.push_back(Literal());

    do {
        // remove ol reason
        analisys_reason.clear();

        // increase activity for conflict clause
        if ( conflict->is_learned() )
            conflict->update_activity(param.clause_activity_update);

        // calculate the reason for the literal p
        // the reason is the set of literals that make this literal false
        for( auto it = p != UNDEF_LIT ? conflict->begin()+1 : conflict->begin();
                it != conflict->end(); ++it)
            analisys_reason.push_back( !(*it) );

        // trace reason of P
        for ( const auto &q : analisys_reason ) {
            if ( ! analisys_seen[q.var()] ) {
                analisys_seen[q.var()] = true;
                if ( decision_levels[q.var()] == current_level() )
                    ++counter;
                else if ( decision_levels[q.var()] > 0 ) {
                    out_learnt.push_back(!q);
                    out_btlevel = max(out_btlevel, decision_levels[q.var()]);
                }
            }
        }
        do {
            p = trail.back();
            conflict = antecedents[p.var()];
            undo_one();
        } while ( ! analisys_seen[p.var()] );
        --counter;
    } while ( counter > 0 );
    out_learnt[0] = !p;

    if (param.enable_conflict_semplification) {
        // try to simplify the conflict clause to learn, with a self subsumption
        // from the reasons of all the literals
        unsigned int i, j;
        for (i = j = 1; i < out_learnt.size(); i++){

            ClausePtr c = antecedents[out_learnt[i].var()];

            if ( c == nullptr ) {
                out_learnt[j++] = out_learnt[i];
                continue;
            }

            for (unsigned int k = 1; k < c->size(); k++) {
                if (!analisys_seen[c->at(k).var()] &&
                        decision_levels[c->at(k).var()] != 0){
                    out_learnt[j++] = out_learnt[i];
                    break;
                }
            }
        }
        out_learnt.resize(j);
    }
}

void SATSolver::undo_one() {
    Literal p = trail.back();
    values[p.var()] = LIT_UNASIGNED;
    antecedents[p.var()] = nullptr;
    decision_levels[p.var()]  = -1;
    order.insert(p.var());
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
        c_ref->update_activity( param.clause_activity_update );
    }

    //  add to the watch list
    watch_list[c_ref->at(0).index()].push_back(Watcher(c_ref));
    watch_list[c_ref->at(1).index()].push_back(Watcher(c_ref));

    return false; // no conflict
}

bool SATSolver::add_clause(vector<Literal>& lits) {
    // build the new clause
    ClausePtr clause;
    bool conflict = new_clause(lits, false, clause);
    // if the clause is a conflict, return immediatly
    if ( conflict ) return true; // conflict
    // clause is nullptr if the new clause is a unit
    if ( clause != nullptr ) {
        clauses.push_back(clause);
        // initialize vsids info
        for ( const auto& l : *clause ) literals_activity[l.index()]+=1.0;

    }
    return false; // no conflict
}

void SATSolver::learn_clause(vector<Literal> & lits) {
    PRINT_VERBOSE("learn clause " << lits << endl);
    // build the new clause, it's never a conflict if the clause is learned
    ClausePtr clause;
    new_clause(lits, true, clause);
    // the learned clause is always a unit, with the unasigned literal in 0
    assign(lits[0],clause);
    PRINT_VERBOSE("address " << clause << endl);
    // if the clause have only one literal, don't add that to the list
    if ( clause != nullptr ) {
        learned.push_back(clause);
        // initialize vsids info
        for ( const auto& l : *clause ) {
            literals_activity[l.index()] += param.clause_activity_update;
            order.increase_activity(l);
        }
    }
}

// Nothing for now
void SATSolver::preprocessing() {}

void SATSolver::remove_from_vect( vector<ClausePtr> &v, ClausePtr c ) {
    for ( auto &i : v ) {
        if ( i == c ) {
            i = v.back();
            v.pop_back();
            return;
        }
    }
    assert_message(false,"removing a nonexistent object object "+c->print());
}

void SATSolver::remove_from_vect( vector<Watcher> &v, ClausePtr c ) {
    for ( auto &i : v ) {
        if ( i.get_clause() == c ) {
            i = v.back();
            v.pop_back();
            return;
        }
    }
    assert_message(false,"removing a nonexistent object object "+c->print());
}

void SATSolver::remove_clause( ClausePtr c ) {
    remove_from_vect( watch_list[c->at(0).index()], c );
    remove_from_vect( watch_list[c->at(1).index()], c );

    Clause::deallocate( c );
}

void SATSolver::literals_activity_decay() {
    // if big value is reached, a normalization is required
    if ( param.literal_activity_update > 1e100 ) {
        for ( auto & l : literals_activity )
            l/=1e100;
        param.literal_activity_update/=1e100;
        order.initialize_heap();
    }
    param.literal_activity_update*=param.literal_decay_factor;
}

void SATSolver::clause_activity_decay() {

    // if big value is reached, a normalization is required
    if ( param.clause_activity_update > 1e100 ) {
        for ( auto & c : learned )
            c->renormalize_activity(param.clause_activity_update);
        param.clause_activity_update = 1.0;
    }

    param.clause_activity_update*=param.clause_decay_factor;
}

void SATSolver::reduce_learned() {
    size_t i = 0, j = 0; // use this indices to compact the vector
    // Remove any clause below this activity
    double  extra_lim = param.clause_activity_update / learned.size();

    // sort learned clause by activity (in ascending order)
    sort(learned.begin(), learned.end(),
            [&](const ClausePtr& l, const ClausePtr& r)
                { return l->get_activity() < r->get_activity(); });

    // remove the first half
    for ( ; i < learned.size()/2 ; ++i ) {
        // keep a clause if is the antecedent of an asignment
        if ( learned[i]->size() == 2 || 
                antecedents[learned[i]->at(0).var()] == learned[i] )
            learned[j++] = learned[i]; // keep the justification
        else
            remove_clause(learned[i]);
    }

    // move the second half in the first half (this effectively delete the
    // low activity clauses)
    for (; i < learned.size(); ++i) {
        // keep a clause if is the antecedent of an asignment
        if ( learned[i]->size() == 2 || 
                antecedents[learned[i]->at(0).var()] == learned[i] ||
                learned[i]->get_activity() >= extra_lim )
            learned[j++] = learned[i]; // keep the justification
        else
            remove_clause(learned[i]);
    }

    // keep only the most active clause
    learned.resize( j );
}

void SATSolver::set_number_of_variable(unsigned int n) {
    // right now, it is possible to set the number of variable only one time
    if ( number_of_variable != 0 )
        // TODO: write a proper interface
        throw std::runtime_error("multiple resize not supported yet");

    number_of_variable = n;

    watch_list.resize( 2 * number_of_variable );
    literals_activity.resize( 2 * number_of_variable, 0.0);

    values.resize(n,LIT_UNASIGNED);
    decision_levels.resize(n,-1);
    antecedents.resize(n,nullptr);
    analisys_seen.resize(n);
    order.set_size( 2 * number_of_variable );
}

void SATSolver::set_log( int l) {
    log_level = l;
}

void SATSolver::set_restarting_multiplier(unsigned int b) {
    param.restart_interval_multiplier = b;
}

void SATSolver::set_conflict_clause_reduction( bool r ) {
    param.enable_conflict_semplification = r;
}

void SATSolver::set_preprocessing(bool p) {
    param.enable_preprocessing = p;
}

void SATSolver::set_restart(bool r) {
    param.enable_restart = r;
}

void SATSolver::set_random_choice( bool r ) {
    param.enable_random_choice = r;
}

void SATSolver::set_deletion(bool d) {
    param.enable_deletion = d;
}

void SATSolver::set_clause_decay(double decay) {
    assert_message( decay > 0.0 && decay <= 1.0, "must be 0.0 < decay ≤ 0.1 ");
    param.clause_decay_factor = 1.0 / decay;
}

void SATSolver::set_literal_decay(double decay) {
    assert_message( decay > 0.0 && decay <= 1.0, "must be 0.0 < decay ≤ 0.1 ");
    param.literal_decay_factor = 1.0 / decay;
}

void SATSolver::set_learning_multiplier( double value ) {
    param.initial_learn_mult = value;
}

void SATSolver::set_learning_increase( double value ) {
    param.percentual_learn_increase = value;
}

inline unsigned int SATSolver::random() {
    return random_kiss();
}

unsigned int SATSolver::random_lcg() { 
    // Linear Congruential Generator
    seed_1 = ( 2477 * seed_1 + 6803 ) % 2147483648;
    return seed_1;
}

unsigned int SATSolver::random_kiss() { 
    // the KISS random number generator

    seed_1 = 69069*seed_1+12345; 

    seed_2 ^= (seed_2<<13);
    seed_2 ^= (seed_2>>17);
    seed_2 ^= (seed_2<<5);

    uint64_t t = 698769069ULL*seed_3+seed_4;

    seed_4 = (t>>32);

    return seed_1+seed_2+(seed_3=seed_4); 
}

} // end namespace Satyricon

