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
        if ( size() == 0 ) return "□";

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
