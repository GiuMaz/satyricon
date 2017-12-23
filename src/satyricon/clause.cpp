#include <algorithm>
#include "assert_message.hpp"
#include "clause.hpp"

namespace Satyricon {

// utility function that compute the hash of a literal, in range 0..63
uint8_t lit_hash( const Literal& l ) {
    return l.atom()%32 + ( l.is_negated() ? 32 : 0 );
}

SATSolver::Clause::Clause(SATSolver& s, std::vector<Literal> lits, bool learn,
        std::shared_ptr<Clause> first, std::shared_ptr<Clause> second) :
    activity(0.0),
    signature(0),
    solver(s),
    literals(lits),
    learned(learn),
    learned_from({first, second})
{
    if ( literals.empty() ) return;
    watch[0] = literals.front();
    watch[1] = literals.back();

    // compute the signature, used for smart subsumption elimination
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
    // make sure that the problematic literal is in watch[0]
    if (solver.get_asigned_value(watch[1]) == LIT_FALSE)
        std::swap(watch[0],watch[1]);

    // if the clause is already solved, nothing need to be moved
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

    // no new literal to watch, reinsert in the old position
    solver.watch_list[l].push_back(shared_from_this());
    // if no new literal is available, the clause must be a unit or a conflict
    if ( solver.get_asigned_value(watch[1]) == LIT_UNASIGNED ) {
        // assign the unit
        solver.assign(watch[1],shared_from_this());
        return false; // no conflict
    }
    else
        return true; // conflict!
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
        while ( it != solver.subsumption[l].end() ) {
            if ( (*it) != shared_from_this() ) continue;
            solver.subsumption[l].erase(it);
            break;
        }
    }
}

std::string SATSolver::Clause::print() const {
    // special simbol for empty clause
    if ( literals.empty() ) return "□";
    std::ostringstream os;
    auto it = literals.begin();
    os << "{" << *it++;
    while ( it != literals.end() ) os << "," << *(it++) ;
    os << "}";
    return os.str();
}

void SATSolver::Clause::print_justification(std::ostream& os,
        const std::string& prefix) const {
    os << print(); // print the clause
    if ( learned ) {
        // if learned, explain how the clause was learned
        assert_message(learned_from[0] != nullptr && learned_from[1] != nullptr,
                "learned_from is nullptr, must be another clause");

        os << " from resolution of " << learned_from[0]->print() <<
            " and " << learned_from[1]->print() << std::endl;

        os << (prefix+"┣╸");
        learned_from[0]->print_justification(os,prefix+"┃ ");
        os << (prefix+"┗╸");
        learned_from[1]->print_justification(os,prefix+"  " );
    }
    else
        // if not learned, simply refer to the original forumal
        os << " from the original formula" << std::endl;
}

bool SATSolver::Clause::propagate(Literal l);

bool SATSolver::Clause::is_learned() const {
    return learned;
}

Literal& SATSolver::Clause::at(size_t pos) {
    return literals[pos];
}

const Literal& SATSolver::Clause::at(size_t pos) const {
    return literals[pos];
}

Literal& SATSolver::Clause::operator[](size_t pos) {
    return at(pos);
}

const Literal& SATSolver::Clause::operator[](size_t pos) const {
    return at(pos);
}

SATSolver::Clause::const_iterator SATSolver::Clause::begin() const {
    return literals.begin();
}

SATSolver::Clause::iterator SATSolver::Clause::begin() {
    return literals.begin();
}

SATSolver::Clause::const_iterator SATSolver::Clause::end() const {
    return literals.end();
}

SATSolver::Clause::iterator SATSolver::Clause::end() {
    return literals.end();
}

std::vector<Literal>& SATSolver::Clause::get_literals() {
    return literals;
}

std::vector<Literal>::size_type SATSolver::Clause::size() const {
    return literals.size();
}

const std::vector<Literal>& SATSolver::Clause::get_literals() const {
    return literals;
}

uint64_t SATSolver::Clause::get_signature() const {
    return signature;
}

} // end namespace Satyricon

