#ifndef SATYRICON_SOLVER_HPP
#define SATYRICON_SOLVER_HPP

#include <queue>
#include <vector>
#include <list>
#include <memory>
#include <unordered_map>
#include "literal.hpp"
#include "vsids.hpp"
#include "log.hpp"

namespace Satyricon {

class SATSolver {

public:

    SATSolver();

    bool solve();
    bool add_clause( const std::vector<Literal>& c );
    void set_number_of_variable(unsigned int n);

    void set_log( Utils::Log l );

    const std::vector<int>& get_model();
    std::string string_model();

    std::string string_conterproof();
private:

    void build_model();
    // forward declaration of support class Clause
    class Clause;
    using WatchMap =
        std::unordered_map<Literal,std::list<std::shared_ptr<Clause> >>;
    using SubsumptionMap =
        std::unordered_map<Literal,std::list<std::shared_ptr<Clause> >>;

    bool learn_clause();

    // get value of a literal
    literal_value get_asigned_value(const Literal & l);

    // assign a literal
    bool assign(Literal l, std::shared_ptr<Clause> c);

    // propage using the queue
    bool unit_propagation();

    // crete ad-hoc data structure, start with a easy implementation
    Literal decide_new_literal();

    // backtrack to a certain level, clear all the value in between
    void backtrack(int level);

    // analyze a conflict clause and create a new clause to be learned
    // and a proper backtrack level
    int conflict_analysis();

    void build_unsat_proof();

    // Preprocess the set of clause
    void preprocessing();
    bool subset(const Clause& inner,const Clause& outer);

    std::vector<std::shared_ptr<Clause> > clauses;
    std::vector<std::shared_ptr<Clause> > learned;
    unsigned int number_of_variable;

    WatchMap watch_list;

    std::vector<literal_value> values;
    std::vector<int> decision_levels;
    std::vector<std::shared_ptr<Clause> > antecedents;
    std::shared_ptr<Clause> conflict_clause;

    std::queue<Literal> propagation_queue;
    unsigned int number_of_assigned_variable;

    // VSIDS
    VSIDS_Info vsids;

    std::vector<Literal> trial;
    int current_level;

    Utils::Log log;

    std::vector<int> model;

    SubsumptionMap subsumption;
};

} // end namespace Satyricon

#endif
