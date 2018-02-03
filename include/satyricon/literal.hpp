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
    LIT_FALSE = 0,
    LIT_UNASIGNED = 1,
    LIT_TRUE = 2
};

/**
 * Literal class.
 * Contain the basic information required to rappresent a literal and
 * some other helpfull method
 */
class Literal {
public:
    // constructor
    Literal(int _value, bool _is_negated) :
        value( _value + _value + (int)_is_negated ) {}
    Literal(): Literal(-1,false) {}

    Literal(const Literal& other) = default;

    // getter
    // TODO: change to sign, revert everything!
    bool sign() const { return value & 1; }
    unsigned int var() const   { return (unsigned int) value >> 1; }
    unsigned int index() const { return (unsigned int) value; }

    // assignment
    Literal& operator=(const Literal& other) = default;
    // comparison operator
    bool operator==(const Literal& rhs) const { return this->value == rhs.value; }
    bool operator!=(const Literal& rhs) const { return this->value != rhs.value; }
    // inversion: return a new literal with the same atom value and
    // reversed polarity
    Literal operator! () const { Literal l; l.value = value ^ 1; return l; }

    // utility print
    std::string print() const { return (sign() ? "-" : "") + std::to_string(var()+1); }

private:
    int value;
};

static const Literal UNDEF_LIT; // undefined literal

// usefull print method
inline std::ostream& operator<<(std::ostream &os, Literal const &l) {
    return os << l.print();
}

inline std::ostream& operator<<(std::ostream &os, std::set<Literal> const &s) {
    if ( s.empty() ) return os << "□";
    auto it = s.begin();
    os << "{" << *it++;
    while ( it != s.end() ) os << "," << *(it++) ;
    return os << "}";
}

inline std::ostream& operator<<(std::ostream &os, std::vector<Literal> const &v) {
    if ( v.empty() ) return os << "□";
    auto it = v.begin();
    os << "{" << *it++;
    while ( it != v.end() ) os << "," << *(it++) ;
    return os << "}";
}

} // end namespace Satyricon

#endif
