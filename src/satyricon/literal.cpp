#include "literal.hpp"

namespace Satyricon {

Literal::Literal() : negated(false), value(0) {}

Literal::Literal(unsigned int _atom, bool _is_negated) :
    negated(_is_negated), value(_atom)
{}

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
    return (negated ? "-" : "") + std::to_string(value+1);
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

