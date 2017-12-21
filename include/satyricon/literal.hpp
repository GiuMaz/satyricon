#ifndef SATYRICON_LITERAL_HPP
#define SATYRICON_LITERAL_HPP

#include <vector>
#include <set>
#include <iostream>

namespace Satyricon {

/**
 * Possible value assigned to a literal
 */
enum literal_value {
    LIT_FALSE,
    LIT_UNASIGNED,
    LIT_TRUE
};

/**
 * Literal class.
 * Contain the basic information required to rappresent a literal and
 * some other helpfull method
 */
class Literal {
public:
    // constructor
    Literal();
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
    int value;
    /*
    unsigned int atom_val;
    bool negated;
    */
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
