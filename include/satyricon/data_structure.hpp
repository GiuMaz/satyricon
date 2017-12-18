#ifndef SATYRICON_DATA_STRUCTURE_HPP
#define SATYRICON_DATA_STRUCTURE_HPP

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
 * literal object.
 * 0 is not a litteral, a literal l < 0 is a negated literal
 */
class Literal {
public:
    Literal(unsigned int _atom, bool _is_negated) :
        atom_val(_atom), negated(_is_negated) {}
    bool is_negated() const { return negated; }
    unsigned int atom() const { return atom_val; }

    Literal& operator=(const Literal& other) {
        atom_val = other.atom_val;
        negated = other.negated;
    }

    bool operator<(const Literal& other) const {
        if ( this->atom() == other.atom() )
            return this->is_negated() < other.is_negated();
        else
            return this->atom() < other.atom();
    }

    std::string print() const {
        return std::string(negated?"-":"")+std::to_string(atom_val+1);
    }

private:
    unsigned int atom_val;
    bool negated;
};

inline std::ostream& operator<<(std::ostream &os, Literal const &l) {
    return os << l.print();
}

inline std::ostream& operator<<(std::ostream &os, std::set<Literal> const &s) {
    os << "[ ";
    for ( const auto& l : s)
        os << l << " ";
    os << "]";
    return os;
}

inline std::ostream& operator<<(std::ostream &os, std::vector<Literal> const &v) {
    os << "[ ";
    for ( const auto& l : v)
        os << l << " ";
    os << "]";
    return os;
}

struct LitHash
{
    size_t operator()(const Literal& k) const {
        return k.is_negated() ? -k.atom() : k.atom();
    }
};

inline Literal operator! (const Literal& l) {
    return Literal(l.atom(), ! l.is_negated());
}

inline bool operator==(const Literal& lhs, const Literal& rhs) {
    return lhs.atom() == rhs.atom() && lhs.is_negated() == rhs.is_negated();
}

inline bool operator!=(const Literal& lhs, const Literal& rhs) {
    return !(lhs == rhs);
}

class VSIDS_Info {
public:
    VSIDS_Info():
        positive(),
        negative(),
        update_value(1.0),
        max_update (std::numeric_limits<double>::max()/(100 + 1)),
        decay_factor(2.0),
        decay_limit(100),
        decay_counter(0) 
    {}

    void set_size( size_t size ) {
        positive.resize(size, 0.0);
        negative.resize(size, 0.0);
    }

    void set_parameter(double decay, int limit) {
        decay_factor = decay;
        decay_limit = limit;
        max_update = std::numeric_limits<double>::max()/(limit + 1);
    }

    void decay() {
        decay_counter++;

        if ( decay_counter >= decay_limit ) {

            decay_counter = 0;

            if (update_value > max_update ) {
                for(auto& i : positive)
                    i/=update_value;
                for(auto& i : negative)
                    i/=update_value;
                update_value = 1.0;
            }

            update_value*=decay_factor;
        }
    }

    Literal select_new(const std::vector<literal_value>& assignment) {
        bool max_negated = false;
        int max_atom = -1;
        double max_value = -1.0;

        for (int i =0; i < positive.size(); ++i)
            if ( positive[i] > max_value && assignment[i] == LIT_UNASIGNED ) {
                max_value =  positive[i];
                max_atom = i;
            }

        for (int i =0; i < negative.size(); ++i)
            if ( negative[i] > max_value && assignment[i] == LIT_UNASIGNED ) {
                max_negated = true;
                max_value =  negative[i];
                max_atom = i;
            }
        assert_message(max_atom >= 0,
                "Unable to find a new literal in VSIDS");
        return Literal(max_atom,max_negated);
    }

    void update(const Literal& lit) {
        if ( lit.is_negated() )
            negative[lit.atom()]+=update_value;
        else
            positive[lit.atom()]+=update_value;
    }

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
