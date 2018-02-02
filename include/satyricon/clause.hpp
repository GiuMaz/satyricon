#ifndef SATYRICON_CLAUSE_HPP
#define SATYRICON_CLAUSE_HPP

#include <string>
#include <memory>
#include <new>
#include <stdlib.h>
#include <sstream>
#include "literal.hpp"
#include "sat_solver.hpp"
#include "assert_message.hpp"

namespace Satyricon {

class Clause final {
using Iterator = Literal *;
using ConstIterator = const Literal *;

private:
    // the learned clause need activity, the original need lit_hash
    union { uint64_t signature; double activity; };
    bool learned  :  1;
    uint64_t _size : 63;

    // utility function that compute the hash of a literal in range 0..63
    uint8_t lit_hash( const Literal& l ) {
        uint8_t body = l.var()%32;
        uint8_t sign = l.sign() ? 32 : 0 ;
        assert_message( (sign+body)>=0 && (sign+body)<=63,
                "literal hash out of bound");
        return static_cast<uint8_t>(body + sign);
    }

    // not to be used directly, but only with allocate function
    Clause(bool l,const std::vector<Literal> &lits) :
        learned(l), _size(lits.size()) {
            std::copy(lits.begin(),lits.end(),this->begin());
            if ( learned )
                activity = 1.0;
            else {
                signature = 0;
                for ( const auto &l : *this )
                    signature |= (1 << lit_hash(l));
            }
    }

public:
    static Clause* allocate(const std::vector<Literal> &lits,
            bool learned = false) {
        auto size = sizeof(Clause) + sizeof(Literal)*lits.size();
        void* memory =  malloc(size);
        return new (memory) Clause(learned,lits);
    }

    static void deallocate(Clause* &c) {
        assert_message( c != nullptr, "freeing a null pointer");
        c->~Clause();
        free(c);
        c = nullptr;
    }
    
    bool locked ( const SATSolver &s ) {
        return s.antecedents[begin()->index()] == this;
    }

    uint64_t size() const { return _size; }
    bool is_learned() const { return learned; }
    double get_activity() const {
        assert_message(is_learned(),"only learned clausole has activity");
        return activity;
    }
    uint64_t get_signature() const {
        assert_message(!is_learned(),"learned clausole had no signature");
        return signature;
    }

    Literal* get_data() {
        return reinterpret_cast<Literal*>(this+1);
    }
    const Literal* get_data() const {
        return reinterpret_cast<const Literal*>(this+1);
    }

    void update_activity(const SATSolver& s) {
            activity+=s.clause_activity_update;
    }

    void renormalize_activity(const SATSolver& s) {
            activity/=s.clause_activity_update;
    }

    bool simplify( const SATSolver &s ) {
        assert_message( s.current_level() == 0,"only at top level" );
        assert_message( s.propagation_queue.empty(),
                "no semplification before propagation");
        size_t j = 0;
        for ( const auto & l : *this ) {
            if ( s.get_asigned_value(l) == LIT_TRUE )
                return true;
            else if ( s.get_asigned_value(l) == LIT_UNASIGNED )
                at(j++) = l;
        }
        _size = j;
        return false;
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
        if ( size() == 0 ) return "□";

        std::ostringstream os;
        auto it = begin();
        os << "{" << *it++;
        while ( it != end() ) { os << "," << *(it++); }
        os << "}";
        return os.str();
    }

    void calcReason(const SATSolver& s, Literal p,
            std::vector<Literal> &out_reason) {
        // invarian: ( p == undef ) || (p == lits[0])
        auto it = begin();
        if ( p != UNDEF_LIT ) ++it;
        for (; it != end(); ++it)
            out_reason.push_back( !(*it) );
        if ( learned ) update_activity(s);
    }

    bool propagate ( SATSolver &s, Literal l ) {

        // make sure the false literal is in position 1
        if ( at(1) != l ) { at(0) = at(1); at(1) = l; }

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
                *it = l;
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

} // end namespace Satyricon

#endif
