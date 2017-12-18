#ifndef SATYRICON_CLAUSE_HPP
#define SATYRICON_CLAUSE_HPP

#include <string>
#include <queue>
#include <list>
#include <set>
#include <memory>
#include <unordered_map>
#include <sstream>
#include "log.hpp"
#include "literal.hpp"
#include "sat_solver.hpp"

namespace Satyricon {

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
