#ifndef SATYRICON_SOLVER_HPP
#define SATYRICON_SOLVER_HPP

#include "data_structure.hpp"
#include <string>
#include <queue>
#include <list>
#include <set>
#include <memory>
#include <unordered_map>
#include <sstream>
#include "log.hpp"
#include "literal.hpp"

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
        std::unordered_map<Literal,std::list<std::shared_ptr<Clause> >,LitHash>;
    using SubsumptionMap =
        std::unordered_map<Literal,std::list<std::shared_ptr<Clause> >,LitHash>;

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

    void build_conterproof();

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
    std::shared_ptr<Clause> first_counterp;
    std::shared_ptr<Clause> second_counterp;

    SubsumptionMap subsumption;

};

/* Clause class
 * A Clause is related to a specific SAT instance.
 * It contein not only the literal vector but also information for efficent
 * analysis of the problem status
 *
 * enable_shared_from_this allow to use the class like a smart pointer of
 * itself, usefull for managing the wath_list
 */
class SATSolver::Clause :
    public std::enable_shared_from_this<SATSolver::Clause> { 
public:

    typedef std::vector<Literal>::iterator iterator;
    typedef std::vector<Literal>::const_iterator const_iterator;
    // disable empy constructor, a clause must refer to a SAT instance
    Clause() = delete;
    Clause(SATSolver& s, std::vector<Literal> lits, bool learn,
            std::shared_ptr<Clause> first  = nullptr,
            std::shared_ptr<Clause> second = nullptr);

    // must remove the clausole from the watched list
    void remove();

    // propagate the information after a literal became false
    // return true if the clause become a conflict
    bool propagate(Literal l);

    bool is_learned() { return learned; }

    void initialize_structure();

    Literal& at(size_t pos) { return literals[pos]; }
    const Literal& at(size_t pos) const { return literals[pos]; }
    Literal& operator[](size_t pos) { return at(pos); }
    const Literal& operator[](size_t pos) const { return at(pos); }
    const_iterator begin() const { return literals.begin(); }
    iterator begin() { return literals.begin(); }
    const_iterator end() const { return literals.end(); }
    iterator end() { return literals.end(); }
    std::vector<Literal>& get_literals() { return literals; }
    std::vector<Literal>::size_type size() { return literals.size(); }
    const std::vector<Literal>& get_literals() const { return literals; }

    std::string print() const;
    uint64_t get_signature() const { return signature; }

    void print_justification(std::ostream& os, const std::string& prefix = "");

private:
    uint64_t signature;
    SATSolver& solver;
    std::vector<Literal> literals;
    bool learned;
    Literal watch[2];
    std::shared_ptr<Clause > learned_from[2];
};

} // end namespace Satyricon

#endif
