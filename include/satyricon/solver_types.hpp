#ifndef SATYRICON_SOLVER_TYPES_HPP
#define SATYRICON_SOLVER_TYPES_HPP

#include <iostream>
#include <memory>
#include <new>
#include <vector>
#include <set>
#include <sstream>
#include <string>
#include <stdlib.h>
#include "assert_message.hpp"

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
    if ( s.empty() ) return os << "(empty)";
    auto it = s.begin();
    os << "{" << *it++;
    while ( it != s.end() ) os << "," << *(it++) ;
    return os << "}";
}

inline std::ostream& operator<<(std::ostream &os, std::vector<Literal> const &v) {
    if ( v.empty() ) return os << "(empty)";
    auto it = v.begin();
    os << "{" << *it++;
    while ( it != v.end() ) os << "," << *(it++) ;
    return os << "}";
}

/**
 * Clause class
 * this class rappresent a clause of at list two literals
 * the clause is intended to be bound to a specific instance of a Solver
 */
class Clause final {
using Iterator = Literal *;
using ConstIterator = const Literal *;

private:
    // the learned clause need activity, the original need lit_hash
    double activity;
    bool learned  :  1;
    uint64_t _size : 31;

    // not to be used directly, but only with allocate function
    Clause(bool l,const std::vector<Literal> &lits) :
        learned(l), _size(lits.size()) {
            std::copy(lits.begin(),lits.end(),this->begin());
            if ( learned ) activity = 1.0;
        }

public:
    static Clause* allocate(const std::vector<Literal> &lits,
            bool learned = false) {
        auto size = sizeof(Clause) + sizeof(Literal)*lits.size();
        void* memory =  malloc(size);
        assert_message( memory != nullptr, "out of memory");
        return new (memory) Clause(learned,lits);
    }

    static void deallocate(Clause* &c) {
        assert_message( c != nullptr, "freeing a null pointer");
        c->~Clause();
        free((void*)c);
        c = nullptr;
    }
    
    uint64_t size() const { return _size; }
    bool is_learned() const { return learned; }
    double get_activity() const {
        assert_message(is_learned(),"only learned clausole has activity");
        return activity;
    }

    Literal* get_data() {
        return reinterpret_cast<Literal*>(this+1);
    }
    const Literal* get_data() const {
        return reinterpret_cast<const Literal*>(this+1);
    }

    Iterator begin() { return get_data(); }
    Iterator end()   { return get_data() + _size; }
    ConstIterator begin() const { return get_data(); }
    ConstIterator end()   const { return get_data() + _size; }
    Literal& at(size_t i) { return get_data()[i]; }
    const Literal& at(size_t i) const { return get_data()[i]; }
    Literal& operator [] (size_t i) { return get_data()[i]; }
    const Literal& operator [] (size_t i) const { return get_data()[i]; }

    std::string print() const {
        // special simbol for empty clause
        if ( size() == 0 ) return "(empty)";

        std::ostringstream os;
        auto it = begin();
        os << "{" << *it++;
        while ( it != end() ) { os << "," << *(it++); }
        os << "}";
        return os.str();
    }

    void update_activity(double value) { activity+=value; }
    void renormalize_activity(double value) { activity/=value; }

    // this metod can be used to reduce the size of the literal
    void shrink( size_t new_size) {
        assert_message( new_size <= _size, "cannot increase size");
        if ( new_size == _size ) return;
        _size = new_size;
    }
};

} // end namespace Satyricon

#endif
