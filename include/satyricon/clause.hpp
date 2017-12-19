#ifndef SATYRICON_CLAUSE_HPP
#define SATYRICON_CLAUSE_HPP

#include <string>
#include <memory>
#include <sstream>
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

    // return true if the clause is a clause learned by a conflict
    // false otherwise
    bool is_learned();

    // initialize strucuture inside the solve that handle this specific
    // clause. It initialize watch_list and subsumption structure
    void initialize_structure();

    // literals access and iteration
    std::vector<Literal>& get_literals();
    const std::vector<Literal>& get_literals() const;
    Literal& at(size_t pos);
    const Literal& at(size_t pos) const;
    Literal& operator[](size_t pos);
    const Literal& operator[](size_t pos) const;
    const_iterator begin() const;
    iterator begin();
    const_iterator end() const;
    iterator end();

    // the clause size is the number of literals inside it
    std::vector<Literal>::size_type size();

    // pretty printing of clause
    std::string print() const;

    // signature used for subsumption of clause
    uint64_t get_signature() const;

    // print how the clause are added inside the sistem
    // if the clasue is one of the original clause it simply print that.
    // if the clause is a learned one it also print the justification
    // of the two parents that generate the clause
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
