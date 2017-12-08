#ifndef SATYRICON_DATA_STRUCTURE_HPP
#define SATYRICON_DATA_STRUCTURE_HPP

#include <vector>

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

/*
 * literal asignement
 */
using Assignment = std::vector<literal_value>;

/**
 * literal object.
 * 0 is not a litteral, a literal l < 0 is a negated literal
 */
using Literal = int;

/* real class
struct Clausole {
    std::vector<Literal> literals;
}
*/
using Clausole = std::vector<Literal>;

/**
 * Formula
 */
struct Formula {
    std::vector<Clausole> clausoles;
    int number_of_clausole;
    int number_of_variable;
};

} // end namespace Satyricon

#endif
