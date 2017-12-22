#include <limits>
#include "vsids.hpp"
#include "assert_message.hpp"

namespace Satyricon {

VSIDS_Info::VSIDS_Info():
    positive(),
    negative(),
    update_value(1.0),
    max_update(std::numeric_limits<double>::max()/100),
    decay_factor(2.0)
{}

void VSIDS_Info::set_size( size_t size ) {
    positive.resize(size, 0.0);
    negative.resize(size, 0.0);
}

void VSIDS_Info::set_parameter(double decay) {
    decay_factor = decay;
}

void VSIDS_Info::renormalize_big_number() {
    for(auto& i : positive)
        i/=update_value;
    for(auto& i : negative)
        i/=update_value;
    update_value = 1.0;
}

void VSIDS_Info::decay() {
    if (update_value > max_update )
        renormalize_big_number();

    update_value*=decay_factor;
}

Literal VSIDS_Info::select_new(const std::vector<literal_value>& assignment) {
    bool max_negated = false;
    int max_atom = -1;
    double max_value = -1.0;

    // find the maximum value.
    for (size_t i =0; i < positive.size(); ++i)
        if ( positive[i] > max_value && assignment[i] == LIT_UNASIGNED ) {
            max_value =  positive[i];
            max_atom = i;
        }

    for (size_t i =0; i < negative.size(); ++i)
        if ( negative[i] > max_value && assignment[i] == LIT_UNASIGNED ) {
            max_negated = true;
            max_value =  negative[i];
            max_atom = i;
        }

    assert_message(max_atom >= 0, "Unable to find a new literal in VSIDS");

    return Literal(max_atom,max_negated);
}

void VSIDS_Info::update(const Literal& lit) {
    if ( lit.is_negated() )
        negative[lit.atom()]+=update_value;
    else
        positive[lit.atom()]+=update_value;
}

} // end of namespace Satyricon

