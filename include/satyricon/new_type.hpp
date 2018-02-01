#ifndef SATYRICON_TYPE_HH
#define SATYRICON_TYPE_HH

/*
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <memory>
#include <new>
#include <stdlib.h>
#include <sstream>
#include "literal.hpp"
#include "sat_solver.hpp"
#include "assert_message.hpp"

//
//using SATSolver = Satyricon::SATSolver;

namespace Satyricon {

// Possible value assigned to a literal
enum literal_value {
    LIT_FALSE = 0,
    LIT_UNASIGNED = 1,
    LIT_TRUE = 2
};



class Literal {
public:
    // constructor
    Literal();
    Literal(unsigned int _atom, bool _is_negated);
    Literal(const Literal& other) = default;

    // getter
    bool is_negated() const { return negated; }
    unsigned int atom() const { return value; }

    // assignment
    Literal& operator=(const Literal& other) = default;
    // comparison operator
    bool operator==(const Literal& rhs) const;
    bool operator!=(const Literal& rhs) const;
    // inversion: return a new literal with the same atom value and
    // reversed polarity
    Literal operator! ();

    // utility print
    std::string print() const;

private:
    // 1 bit for the sign, 31 for the value
    bool negated       :  1;
    unsigned int value : 31;
};
// usefull print method
std::ostream& operator<<(std::ostream &os, Literal const &l);

class ClauseCompact {
using Iterator = Literal *;
using ConstIterator = const Literal *;

private:
    // the learned clause need activity, the original need lit_hash
    union { uint64_t hash; double activity; };
    bool learned  :  1;
    uint64_t size : 63;

    // utility function that compute the hash of a literal in range 0..63
    uint8_t lit_hash( const Literal& l ) {
        uint8_t body = l.atom()%32;
        uint8_t sign = l.is_negated() ? 32 : 0 ;
        assert_message( (sign+body)>=0 && (sign+body)<=63,
                "literal hash out of bound");
        return static_cast<uint8_t>(body + sign);
    }

    // not to be used directly, but only with allocate function
    ClauseCompact(bool l,const std::vector<Literal> &lits) :
        learned(l), size(lits.size()) {
            std::copy(lits.begin(),lits.end(),this->begin());
            if ( learned )
                activity = 1.0;
            else {
                hash = 0;
                for ( const auto &l : *this )
                    hash |= (1 << lit_hash(l));
            }
    }

public:
    static ClauseCompact* allocate(const std::vector<Literal> &lits,
            bool learned = false) {
        auto size = sizeof(ClauseCompact) + sizeof(Literal)*lits.size();
        std::cout << "size: " << size << std::endl;
        void* memory =  malloc(size);
        return new (memory) ClauseCompact(learned,lits);
    }

    static void deallocate(ClauseCompact* &c) {
        if (!c) return; // don't free a null pointer
        c->~ClauseCompact();
        free(c);
        c = nullptr;
    }
    
    uint64_t get_size()     const { return size; }
    bool     is_learned()   const { return learned; }
    double   get_activity() const { return activity; }
    uint64_t get_hash()     const { return hash; }

    Literal* get_data() {
        return reinterpret_cast<Literal*>(this+1);
    }
    const Literal* get_data() const {
        return reinterpret_cast<const Literal*>(this+1);
    }

    void update_activity(const SATSolver& s) {
            activity+=s.clause_activity_update;
    }

    bool simplify( const SATSolver &s ) {

        size_t j = 0;
        for ( const auto & l : *this ) {
            if ( s.get_asigned_value(l) == LIT_TRUE )
                return true;
            else if ( s.get_asigned_value(l) == LIT_UNASIGNED )
                at(j++) = l;
        }
        size = j;
        return false;
    }

    Iterator begin() { return get_data(); }
    Iterator end()   { return get_data() + size; }
    ConstIterator begin() const { return get_data(); }
    ConstIterator end()   const { return get_data() + size; }
    Literal& at(size_t i) { return get_data()[i]; }
    const Literal& at(size_t i) const { return get_data()[i]; }
    Literal& operator [] (size_t i) { return get_data()[i]; }
    const Literal& operator [] (size_t i) const { return get_data()[i]; }

    void calcReason(const SATSolver& s, Literal p, std::vector<Literal> out_reason) {
        // invarian: ( p == undef ) || (p == lits[0])
        auto it = begin();
        if ( p != UNDEF_LIT ) ++it;
        for (; it != end(); ++it)
            out_reason.push_back(!*it);
        if ( learned ) update_activity(s);

    }

    bool propagate ( const SATSolver &s, Literal l ) {

        // make sure the false literal is in position 1
        if ( at(1) == !l ) { at(0) = at(1); at(1) = !l; }

        // if the clause is already solved, nothing need to be moved
        if (s.get_asigned_value(at(0)) == LIT_TRUE) {
            // reinsert inside the previous watch list
            s.watch_list[l.index()].push_back(this);
            return false; // no conflict
        }

        // search a new literal to watch
        for ( auto it = begin()+2; it != end(); ++it ) {
            if ( s.get_asigned_value(*it) != LIT_FALSE ) {
                // foundt a valid literal, move the watch_list
                at(1) = *it;
                *it = !l;
                s.watch_list[at(1).index()].push_back(this);
                return false; // no conflict
            }
        }

        // no new literal to watch, reinsert in the old position
        s.watch_list[l.index()].push_back(this);
        // the clause must be a conflict or a unit, try to assign the value
        return s.assign(at(0),this);
    }
};


}
*/
#endif
