#ifndef SATYRICON_SOLVER_HPP
#define SATYRICON_SOLVER_HPP

#include "data_structure.hpp"
#include <string>
#include <queue>
#include <list>
#include <set>

namespace Satyricon {

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
    SATSolver(Formula& f);
    ~SATSolver() {};

    Solution solve();

private:

    /* useless ???
    enum clause_status_val {
        CLAUSE_SAT, CLAUSE_UNSAT, CLAUSE_UNIT, CLAUSE_UNRESOLVED };
    */

    void assign_decision(Literal l, int level);
    bool assign_infered(Literal l, int level, int antecedent);
    bool unit_propagation(int level);
    Literal decide_new_literal();
    literal_value get_asigned_value(const Literal & l);
    int conflict_analysis();

    Formula & formula;

    std::vector<literal_value> values;
    std::vector<int> decision_levels;
    std::vector<int> antecedents;
    std::array<std::vector<std::list<size_t> >,2> watch_list;
    std::queue<Literal> unit_queue;
    unsigned int number_of_assigned_variable;
    std::vector<int> vsids_positive;
    std::vector<int> vsids_negative;

};

} // end namespace Satyricon

#endif
