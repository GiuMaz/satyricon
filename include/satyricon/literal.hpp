#ifndef SATYRICON_LITERAL_HPP
#define SATYRICON_LITERAL_HPP

#include <vector>
#include <set>
#include <array>
#include <queue>
#include <memory>
#include <limits>
#include "assert_message.hpp"

namespace Satyricon {

enum literal_value {
    LIT_ZERO,
    LIT_UNASIGNED,
    LIT_ONE
};

/**
 * Literal object.
 */
class Literal {
public:
    // constructor
    Literal(unsigned int _atom, bool _is_negated);

    // getter
    bool is_negated() const;
    unsigned int atom() const;

    // assignment
    Literal& operator=(const Literal& other);
    // comparison operator
    bool operator==(const Literal& rhs) const;
    bool operator!=(const Literal& rhs) const;
    // inversion: return a new literal with the same atom value and
    // reversed polarity
    Literal operator! ();

    // utility print
    std::string print() const;

private:
    unsigned int atom_val;
    bool negated;
};

// usefull print method
std::ostream& operator<<(std::ostream &os, Literal const &l);
std::ostream& operator<<(std::ostream &os, std::set<Literal> const &s);
std::ostream& operator<<(std::ostream &os, std::vector<Literal> const &v);

// support hash class, usefull for unordered_map (hash map)
struct LitHash
{
    size_t operator()(const Literal& k) const {
        return k.is_negated() ? -k.atom() : k.atom();
    }
};

} // end namespace Satyricon

#endif
