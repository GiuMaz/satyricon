#ifndef SATYRICON_VSIDS_HPP
#define SATYRICON_VSIDS_HPP

#include <vector>
#include "solver_types.hpp"

namespace Satyricon {

/**
 * Data structure with information about the VSIDS decision heuristic.
 */
class VSIDS_Info {
public:
    VSIDS_Info();

    // set the number of variable to track
    void set_size( size_t size );

    // set the decay value. It multiply the activity of everty literal
    // every time a conflict is found. should be between 0.0 and 1.0
    void set_parameter(double decay);

    // decay all variable, it's only a O(1) operation in fact
    void decay();

    // return the literal with the greater activity value
    Literal select_new(const std::vector<literal_value>& assignment);

    // update the activity of a single literal
    void update(const Literal& lit);

private:
    // utility function for managing high number
    void renormalize_big_number();

    // Literal activity values
    std::vector<double> positive;
    std::vector<double> negative;

    // support value
    double update_value;
    double decay_factor;
};

} // end namespace Satyricon

#endif
