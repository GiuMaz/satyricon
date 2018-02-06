#ifndef SATYRICON_VSIDS_HPP
#define SATYRICON_VSIDS_HPP

#include <limits>
#include <vector>
#include "solver_types.hpp"
#include "assert_message.hpp"

namespace Satyricon {

/**
 * Data structure with information about the VSIDS decision heuristic.
 */
class VSIDS_Info {

public:
    VSIDS_Info():
        positive(),
        negative(),
        update_value(1.0),
        decay_factor( 1.0 / 0.95)
    {}

    // set the number of variable to track
    void set_size( size_t size ) {
        positive.resize(size, 0.0);
        negative.resize(size, 0.0);
    }

    // set the decay value. It multiply the activity of everty literal
    // every time a conflict is found. should be between 0.0 and 1.0
    void set_parameter(double decay) {
        assert_message(decay > 0.0 && decay <= 1.0,"must be 0.0 < decay ≤ 0.1");
        decay_factor = 1.0 / decay;
    }

    // decay all variable, it's only a O(1) operation in fact
    void decay() {
        if ( update_value > 1e100 )
            renormalize_big_number();
        update_value*=decay_factor;
    }

    // return the literal with the greater activity value
    Literal select_new(const std::vector<literal_value>& assignment) {
        bool max_negated = false;
        unsigned int max_atom = 0;
        double max_value = -1.0;

        // find the maximum value.
        for (unsigned int i =0; i < positive.size(); ++i)
            if ( positive[i] > max_value && assignment[i] == LIT_UNASIGNED ) {
                max_value =  positive[i];
                max_atom = i;
            }

        for (unsigned int i =0; i < negative.size(); ++i)
            if ( negative[i] > max_value && assignment[i] == LIT_UNASIGNED ) {
                max_negated = true;
                max_value =  negative[i];
                max_atom = i;
            }

        assert_message(max_value >= 0.0,
                "Unable to find a new literal in VSIDS");

        return Literal(max_atom,max_negated);
    }

    // update the activity of a single literal
    void update(const Literal& lit) {
        if ( lit.sign() )
            negative[lit.var()]+=update_value;
        else
            positive[lit.var()]+=update_value;
    }

private:
    // utility function for managing high number
    void renormalize_big_number() {
        for(auto& i : positive)
            i/=update_value;
        for(auto& i : negative)
            i/=update_value;
        update_value = 1.0;
    }

    // Literal activity values
    std::vector<double> positive;
    std::vector<double> negative;

    // support value
    double update_value;
    double decay_factor;
};

} // end namespace Satyricon

#endif
