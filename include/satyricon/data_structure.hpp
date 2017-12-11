#ifndef SATYRICON_DATA_STRUCTURE_HPP
#define SATYRICON_DATA_STRUCTURE_HPP

#include <vector>
#include <array>
#include <queue>

namespace Satyricon {

/**
 * Literal value, the sequent property hold:
 * LIT_ZERO < LIT_UNKNOWN < LIT_ONE
 * LIT_ONE - LIT_UNKNOWN = LIT_UNKNOWN
 * LIT_ONE - LIT_ONE = LIT_ZERO
 * LIT_ONE - LIT_ZERO = LIT_ONE
 * ATTENTION: it is possible that LIT_ONE != 1 (the c++ int value), etc...
 */
enum literal_value {
    LIT_ZERO = 0,
    LIT_UNASIGNED = 1,
    LIT_ONE = 2
};

// enforce the property at compile time
static_assert ( LIT_ZERO < LIT_UNASIGNED,
        "must be LIT_ZERO < LIT_UNASIGNED < LIT_ONE");
static_assert ( LIT_UNASIGNED < LIT_ONE,
        "must be LIT_ZERO < LIT_UNASIGNED < LIT_ONE");
static_assert ( (LIT_ONE - LIT_ZERO) == LIT_ONE,
        "must be LIT_ONE - LIT_ZERO = LIT_ONE");
static_assert ( (LIT_ONE - LIT_UNASIGNED) == LIT_UNASIGNED,
        "must be LIT_ONE - LIT_UNASIGNED = LIT_UNASIGNED");
static_assert ( (LIT_ONE - LIT_ONE) == LIT_ZERO,
        "must be LIT_ONE - LIT_ONE = LIT_ZERO");

/**
 * literal object.
 * 0 is not a litteral, a literal l < 0 is a negated literal
 */
class Literal {
public:
    Literal(unsigned int _atom, bool _is_negated) : atom_val(_atom), negated(_is_negated) {}
    bool is_negated() const { return negated; }
    unsigned int atom() const { return atom_val; }

    Literal& operator=(const Literal& other) {
        atom_val = other.atom_val;
        negated = other.negated;
    }


private:
    unsigned int atom_val;
    bool negated;
};

inline bool operator==(const Literal& lhs, const Literal& rhs) {
    return lhs.atom() == rhs.atom() && lhs.is_negated() == rhs.is_negated();
}

inline bool operator!=(const Literal& lhs, const Literal& rhs) {
    return !(lhs == rhs);
}

/* Clause class
*/
struct Clause {
    std::vector<Literal> literals;
    std::vector<Literal>::size_type watch[2];
};
//using Clause = std::vector<Literal>;

/**
 * Formula
 */
class Formula {
public:
    std::vector<Clause> clausoles;
    int number_of_clausole;
    int number_of_variable;
};

} // end namespace Satyricon
#endif
