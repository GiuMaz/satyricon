#ifndef SATYRICON_SOLVER_HPP
#define SATYRICON_SOLVER_HPP

#include "data_structure.hpp"
#include <string>
#include <queue>
#include <list>
#include <set>

namespace Satyricon {

class SATSolver;

class Solution {
public:
    Solution(bool _is_sat, const std::string& _solution) :
        satisaible(_is_sat), solution(_solution) {}

    bool is_sat() { return satisaible; }
    const std::string& get_solution() { return solution; }

private:
    bool satisaible;
    std::string solution;
};

class SATSolver {

public:

    SATSolver();

    Solution solve();
    void add_clause( std::vector<Literal> c );

private:
    class Clause;

    // unite the two kind of asignment in a signle function
    void assign_decision(Literal l, int level);
    bool assign_infered(Literal l, int level, int antecedent);

    // propage using the queue
    bool unit_propagation(int level);

    // crete ad-hoc data structure, start with a easy implementation
    Literal decide_new_literal();

    // TODO: maybe have an easy asignment type...
    literal_value get_asigned_value(const Literal & l);

    // TODO: probably the biggest work... understand how to propagate...
    int conflict_analysis();

    std::vector<Clause> clauses;
    //std::vector<Clause> learnt; // required ?
    int number_of_clausole;
    int number_of_variable;

    std::vector<literal_value> values;
    std::vector<int> decision_levels;
    std::vector<int> antecedents;
    std::array<std::vector<std::list<size_t> >,2> watch_list;
    std::queue<Literal> unit_queue;
    unsigned int number_of_assigned_variable;

    // TODO: create ad-hoc Class for storing that
    std::vector<int> vsids_positive;
    std::vector<int> vsids_negative;

};

/* Clause class
 * A Clause is related to a specific SAT instance.
 * It contein not only the literal vector but also information for efficent
 * analysis of the problem status
 */
class SATSolver::Clause { 
public:

    /* probably useless
    typedef typename std::vector<Literal>::iterator iterator;
    typedef typename std::vector<Literal>::const_iterator const_iterator;
    */

    // disable empy constructor, a clause must refer to a SAT instance
    Clause() = delete;

    // TODO: constructor must add the element to the watched list
    Clause(SATSolver& s, std::vector<Literal> lits) :
        solver(s), literals(lits)
    {
        // TODO: check that the clause are valid
    }

    // must remove the clausole from the watched list
    // TODO: write proper remove
    void remove() {}
    ~Clause() { remove(); }

    // propagate the information.
    // it's true if a conflict is found
    // TODO: implement the method
    bool propagate (Literal l) {
        // move the watched literal
        // if unit, add to unit_queue
        // reinsert inside a watched list
        return false;
    }

    /* interfaccia minisat
    void cal_reason (Literal l, vector<Literal> out_reason); must be defined
    */

    /* probably useless
    iterator begin() { return literals.begin(); }
    iterator end()   { return literals.end();   }
    const_iterator begin() const { return literals.begin(); }
    const_iterator end() const   { return literals.end();   }
    */

private:
    std::vector<Literal> literals;
    std::vector<Literal>::size_type watch[2];
    SATSolver& solver;
};

} // end namespace Satyricon

#endif
