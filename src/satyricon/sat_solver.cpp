#include "sat_solver.hpp"
#include "vsids.hpp"
#include "assert_message.hpp"
#include <array>
#include <queue>
#include <algorithm>
#include <tuple>
#include "log.hpp"


using namespace std;
using namespace Satyricon;
using namespace Utils;

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
    first_counterp(),
    second_counterp(),
    subsumption()
{}

bool SATSolver::solve() {
    unsigned long long conflict_counter = 0;
    preprocessing();
    log.normal << "begin solve" << endl;
    while ( true ) {

        log.verbose << "propagate at level " << current_level << endl;
        bool conflict = unit_propagation();
        if ( conflict ) {
            conflict_counter++;
            if ( conflict_counter % 1000 == 0 )
                log.normal << "conflict: " << conflict_counter << endl;

            if ( current_level == 0 ) {
                log.verbose << "conflict at level 0, build counterproof\n";
                build_conterproof();
                return false; // unsolvable conflict
            }

            auto backtrack_level  = conflict_analysis();
            backtrack( backtrack_level );
            learn_clause();
            vsids.decay();
        }
        else {

            if ( number_of_assigned_variable == number_of_variable ) {
                build_model();
                return true; // found a model
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
        ++val;
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

void SATSolver::build_conterproof() {

    for( auto it = trial.rbegin(); it != trial.rend(); ++it) {

        if (find(conflict_clause->begin(), conflict_clause->end(), !(*it)) != conflict_clause->end()) {

            log.verbose << "\t" << conflict_clause->print() << " | "
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
            if ( new_lit.empty() ) { // foundt the empty clause
                first_counterp = conflict_clause;
                second_counterp = antecedents[it->atom()];
                break;
            }
            else {
                auto new_ptr = make_shared<Clause>(*this,new_lit, true, conflict_clause, antecedents[it->atom()]);
                conflict_clause = new_ptr;
            }
        }
    }
}

int SATSolver::conflict_analysis() {

    log.verbose << "learning new clausole from " << conflict_clause->print() << endl;

    int count = 0;
    for ( const auto& l : *conflict_clause )
        if ( decision_levels[l.atom()] == current_level )
            ++count;

    // build the conflict clause
    for( auto it = trial.rbegin(); it != trial.rend() && count >= 2; ++it) {

        if ( decision_levels[it->atom()] < current_level ) break;

        if (find(conflict_clause->begin(), conflict_clause->end(), !(*it)) != conflict_clause->end()) {
            assert_message( antecedents[ it->atom() ] != nullptr,
                    "Unexpected decision literal");

            log.verbose << "\t" << conflict_clause->print() << " | "
                << antecedents[it->atom()]->print();
            count--;

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
            auto new_ptr = make_shared<Clause>(*this,new_lit, true, conflict_clause, antecedents[it->atom()]);
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

    // look for the assertion litteral
    for ( const auto& l : *conflict_clause ) {
        if ( get_asigned_value(l) == LIT_UNASIGNED ) {
            assign( l, learned.back());
            break;
        }
    }

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

    assert_message(backtrack_level < current_level,
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

SATSolver::Clause::Clause(SATSolver& s, vector<Literal> lits, bool learn,
        std::shared_ptr<Clause> first, std::shared_ptr<Clause> second) :
    solver(s), literals(lits), learned(learn),watch({lits.front(),lits.back()}), learned_from({first,second})
{
    // lambda expression that compute an hash from 0 to 63 for every literal
    auto lit_hash = [](Literal l) {
        return l.atom()%32 + (l.is_negated() ?  32 : 0); };

    // compute the signature, used for smart subsumption elimination
    signature = 0;
    for ( auto l : literals )
        signature|=(1 << lit_hash(l));

}

void SATSolver::Clause::initialize_structure() {
    // watched literal
    solver.watch_list[watch[0]].push_back(shared_from_this());
    solver.watch_list[watch[1]].push_back(shared_from_this());

    // preprocessing with subsumption
    for ( const auto& l : literals )
        solver.subsumption[l].push_back(shared_from_this());
}

bool SATSolver::Clause::propagate(Literal l) {
    // move the wrong literal in 0
    if (solver.get_asigned_value(watch[1]) == LIT_FALSE)
        std::swap(watch[0],watch[1]);

    if (solver.get_asigned_value(watch[1]) == LIT_TRUE) {
        // reinsert inside the previous watch list
        solver.watch_list[l].push_back(shared_from_this());
        return false; // no conflict
    }

    // search a new literal to watch
    for ( auto u : literals ) {
        if (solver.get_asigned_value(u) != LIT_FALSE && u != watch[1]) {
            // foundt a valid literal, move the watch_list
            watch[0] = u;
            solver.watch_list[u].push_back(shared_from_this());
            return false; // no conflict
        }
    }

    // no new literal to watch, reinsert in the old position and check
    // if the clause is a unit or a conflict
    solver.watch_list[l].push_back(shared_from_this());

    if ( solver.get_asigned_value(watch[1]) == LIT_UNASIGNED ) {
        // assign the unit
        solver.assign(watch[1],shared_from_this());
        return false; // no conflict
    }
    else
        return true; // conflict!
}

bool SATSolver::subset( const Clause& inner,const Clause& outer) {
    // prefilter using signature
    if ( (inner.get_signature() & ~outer.get_signature()) != 0 )
        return false;
    // more expansive but precise inclusion check
    for ( const auto& l : inner )
        if ( find(outer.begin(), outer.end(), l) == outer.end() )
            return false;

    return true;
}

void SATSolver::preprocessing() {

    size_t initial_size = clauses.size();
    log.normal << "preprocessing " << initial_size << " clauses\n";

    for (size_t i = 0; i < clauses.size() ; ++i) {
        // find minimum
        Literal min = clauses[i]->at(0);
        size_t min_size = subsumption[min].size();

        for ( const auto& l : *clauses[i] ) {
            if ( subsumption[l].size() < min_size ) {
                min_size = subsumption[l].size();
                min = l;
            }
        }

        for ( size_t j = 0; j < clauses.size(); ++j ) {
            if ( clauses[j] != clauses[i] &&
                    clauses[i]->size() <= clauses[j]->size() &&
                    subset(*clauses[i],*clauses[j]) ) {

                log.verbose << "\tsubsume " << clauses[j]->print() <<
                    " from " << clauses[i]->print() << endl;
                clauses.erase(clauses.begin()+j);
                if ( j < i ) --i;
            }
        }
    }
    log.normal << "removed " << (initial_size - clauses.size()) << " clauses\n";
}

string SATSolver::string_conterproof() {
    ostringstream oss;
    oss << "clause □ (empty) from resolution of " << first_counterp->print() << " and " <<
        second_counterp->print() << endl;
    oss << "┣╸";
    first_counterp->print_justification(oss, "┃ ");
    oss << "┗╸";
    second_counterp->print_justification(oss,"  ");
    return oss.str();
}

void SATSolver::Clause::remove() {

    // remove from watch_list
    auto it = solver.watch_list[watch[0]].begin();
    while ( it != solver.watch_list[watch[0]].end() )
        if ( (*it) == shared_from_this() ) {
            solver.watch_list[watch[0]].erase(it);
            break;
        }
    it = solver.watch_list[watch[1]].begin();
    while ( it != solver.watch_list[watch[1]].end() )
        if ( (*it) == shared_from_this() ) {
            solver.watch_list[watch[1]].erase(it);
            break;
        }

    // remove from subsumption info
    for ( const auto& l : literals ) {
        auto it = solver.subsumption[l].begin();
        while ( it != solver.subsumption[l].end() )
            if ( (*it) == shared_from_this() ) {
                solver.subsumption[l].erase(it);
                break;
            }
    }
}

std::string SATSolver::Clause::print() const {
    if ( literals.empty() ) return "□";
    std::ostringstream os;
    auto it = literals.begin();
    os << "{" << *it++;
    while ( it != literals.end() ) os << "," << *(it++) ;
    os << "}";
    return os.str();
}

void SATSolver::Clause::print_justification(std::ostream& os, const std::string& prefix) {
    os << "clause " << print();
    if ( learned ) {
        os << " from resolution of " << learned_from[0]->print() <<
            " and " << learned_from[1]->print() << std::endl;
        assert_message( learned_from[0] != nullptr && learned_from[1] != nullptr,
                "learned is nullptr, must be another clause for learned");
        os << (prefix+"┣╸");
        learned_from[0]->print_justification(os,prefix+"┃ ");
        os << (prefix+"┗╸");
        learned_from[1]->print_justification(os,prefix+"  " );
    }
    else
        os << " from the original formula" << std::endl;
}
