#include "sat_solver.hpp"
#include "data_structure.hpp"
#include "assert_message.hpp"
#include <array>
#include <queue>

using namespace std;
using namespace Satyricon;

SATSolver::SATSolver(Formula& f):
    formula(f),
    values(f.number_of_variable+1, LIT_UNASIGNED),
    decision_levels(f.number_of_variable+1,-1),
    antecedents(f.number_of_variable+1,-1),
    watch_list({vector<list<size_t> >(f.number_of_variable+1),
    vector<list<size_t> >(f.number_of_variable+1)}),
    number_of_assigned_variable(0),
    vsids_positive(f.number_of_variable+1,0),
    vsids_negative(f.number_of_variable+1,0)
{
    // initialize watch_list
    for (size_t i = 0; i < f.clausoles.size(); ++i) {
        for (const auto & l : f.clausoles[i].literals ) {
            watch_list[!l.is_negated()][l.atom()].push_back(i);
        }
    }

    // initialize watcher, find the unit
    for ( auto c : f.clausoles ) {

        c.watch[0] = 0;
        c.watch[1] = c.literals.size() - 1;

        // if the first literal is equal to the last one, it must be a unit
        if (c.watch[0] == c.watch[1]) unit_queue.push(c.literals[0]);
    }

    // initialize VSIDS
    for (const auto& clause : formula.clausoles) {
        for (const auto& lit : clause.literals) {
            if ( ! lit.is_negated() ) ++vsids_positive[ lit.atom() ];
            else ++vsids_negative[ lit.atom() ];
        }
    }
}

literal_value SATSolver::get_asigned_value(const Literal & l) {
    switch (values[l.atom()]) {
        case LIT_ONE:
            return l.is_negated() ? LIT_ZERO : LIT_ONE;
        case LIT_ZERO:
            return l.is_negated() ? LIT_ONE  : LIT_ZERO;
        case LIT_UNASIGNED:
            return LIT_UNASIGNED;
    }
}

bool SATSolver::unit_propagation(int level) {
    while ( ! unit_queue.empty() ) {
        // fetch an element from the queue
        auto l = unit_queue.back(); unit_queue.pop();

        // assign the element. the assignment can cause a conflict and/or add
        // new units to the unit_queue
        bool found_conflict = assign_infered( l, 0,0);
        if ( found_conflict ) return true; // found a conflict
    }
    return false; // no conflict
}

void SATSolver::assign_decision(Literal l, int level) {
}

bool SATSolver::assign_infered(Literal l, int level,int antecedent) {
    ++number_of_assigned_variable;
    values[l.atom()] = l.is_negated() ? LIT_ZERO : LIT_ONE;
    decision_levels[l.atom()] = level;
    antecedents[l.atom()] = antecedent;  // TODO: riguardare definizione corretta di antecedente

    // update watched literals

    auto i  = watch_list[l.is_negated()][l.atom()].begin();
    while ( i !=watch_list[l.is_negated()][l.atom()].end() ) {

        auto& c = formula.clausoles[*i];

        // if one watched literal is true, no change are required
        if ( get_asigned_value(c.literals[c.watch[0]]) == LIT_ONE ||
                get_asigned_value(c.literals[c.watch[1]]) == LIT_ONE) {
            ++i;
            continue;
        }

        auto to_move = c.literals[c.watch[0]] == Literal(l.atom(), !l.is_negated()) ? 0 : 1;
        auto other = 1 - to_move;

        bool find = false;
        for (auto pos = 0; pos < c.literals.size(); ++pos) {
            if (get_asigned_value(c.literals[pos]) == LIT_ZERO) continue;
            if ( pos == c.watch[other] ) continue;

            // find a new literal to watch
            find = true;
            c.watch[to_move] = pos;
            auto new_lit = c.literals[pos];
            watch_list[new_lit.is_negated()][new_lit.atom()].push_back(*i);
            watch_list[l.is_negated()][l.atom()].erase(i++);
            break;
        }

        if ( ! find ) { // could be unit or conflict
            if ( get_asigned_value(c.literals[c.watch[other]]) == LIT_ZERO )
                return true; // found a conflict

            else {
                // otherwise add the new unit
                unit_queue.push(c.literals[c.watch[other]]);
                ++i;
            }
        }
    }

    return false;
}

Literal SATSolver::decide_new_literal() {
    Literal decision = {0,false};
    int max_count = -1;

    for ( auto i = 1; i < vsids_positive.size(); ++i ) {
        if ( vsids_positive[i] > max_count &&
                get_asigned_value(Literal(i,false)) == LIT_UNASIGNED ) {
            max_count = vsids_positive[i];
            decision = Literal(i,false);
        }
    }

    for ( auto i = 1; i < vsids_negative.size(); ++i ) {
        if ( vsids_negative[i] > max_count &&
                get_asigned_value(Literal(i,true)) == LIT_UNASIGNED ) {
            max_count = vsids_negative[i];
            decision = Literal(i,true);
        }
    }

    //assert_message( decision != 0, "No possible decision in VSIDS");
    return decision;
}

int SATSolver::conflict_analysis() {
    /* backjumpint not implemented yet...
    */
    return 0;
}

Solution SATSolver::solve() {
    // TEMPORARY use DPLL

    int decision_level = 0;
    if (unit_propagation(decision_level) == true)
        return Solution(false,"");

    /*
    while ( number_of_assigned_variable < formula.number_of_variable ) {

        // chose the new assignment (using some euristic)
        auto new_lit = decide_new_literal();
        assign_decision(new_lit, decision_level);

        // unit propagation
        bool found_conflict = unit_propagation(decision_level);
        // in case of conflict during the unit propagation, backjump
        if ( found_conflict ) {
            decision_level = backjump_level;
        }
    }
    return Solution(true,"");
    */

    /* CDCL
    int decision_level = 0;
    //preprocessor(f);
    if( unit_propagation(decision_level) == true)
        return Solution(false,"");

    while ( number_of_assigned_variable < formula.number_of_variable ) {

        ++decision_level; // increase decision level

        // chose the new assignment (using some euristic)
        auto new_lit = decide_new_literal();
        assign_decision(new_lit, decision_level);

        // unit propagation
        bool found_conflict = unit_propagation(decision_level);
        // in case of conflict during the unit propagation, backjump
        if ( found_conflict ) {
            int backjump_level = conflict_analysis(); // learn a new clausole
            if ( backjump_level < 0 ) return Solution(false,""); // UNSAT
            //Backtrack(f,a,b); // backjumping
            decision_level = backjump_level;
        }
    }
    return Solution(true,"");
    */
}

