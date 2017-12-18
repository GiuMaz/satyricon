#ifndef SATYRICON_VSIDS_HPP
#define SATYRICON_VSIDS_HPP

#include <vector>
#include "literal.hpp"

namespace Satyricon {

/**
 * Data structure with information about the VSIDS decision heuristic.
 */
class VSIDS_Info {
public:
    VSIDS_Info();

    // set the number of variable to track
    void set_size( size_t size );

    void set_parameter(double decay, int limit);

    void decay();

    Literal select_new(const std::vector<literal_value>& assignment);

    void update(const Literal& lit);

private:
    std::vector<double> positive;
    std::vector<double> negative;
    double update_value;
    double max_update;

    double decay_factor;
    int decay_limit;
    int decay_counter;
};

} // end namespace Satyricon

#endif
