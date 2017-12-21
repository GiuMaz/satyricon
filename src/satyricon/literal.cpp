#include "literal.hpp"

namespace Satyricon {

Literal::Literal() : value(0) {}

Literal::Literal(unsigned int _atom, bool _is_negated) :
    //atom_val(_atom), negated(_is_negated)
    value( _is_negated ? -_atom-1 : _atom+1 )
{}

bool Literal::is_negated() const {
    //return negated;
    return value < 0;
}

unsigned int Literal::atom() const {
    //return atom_val;
    return abs(value)-1;
}

Literal& Literal::operator=(const Literal& other) {
    //atom_val = other.atom_val;
    //negated  = other.negated;
    value = other.value;
    return *this;
}

Literal Literal::operator! () {
    return Literal(atom(), ! is_negated());
}

bool Literal::operator==(const Literal& rhs) const {
    return atom() == rhs.atom() && is_negated() == rhs.is_negated();
}

bool Literal::operator!=(const Literal& rhs) const {
    return !(*this == rhs);
}

std::string Literal::print() const {
    //return std::string(negated?"-":"")+std::to_string(atom_val+1);
    return std::to_string(value);
}

std::ostream& operator<<(std::ostream &os, Literal const &l) {
    return os << l.print();
}

std::ostream& operator<<(std::ostream &os, std::set<Literal> const &s) {
    if ( s.empty() ) return os << "□";
    auto it = s.begin();
    os << "{" << *it++;
    while ( it != s.end() ) os << "," << *(it++) ;
    return os << "}";
}

std::ostream& operator<<(std::ostream &os, std::vector<Literal> const &v) {
    if ( v.empty() ) return os << "□";
    auto it = v.begin();
    os << "{" << *it++;
    while ( it != v.end() ) os << "," << *(it++) ;
    return os << "}";
}

} // end of namespace Satyricon

