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

typedef int var;

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
    Literal(int v, bool is_signed) :
        value( v + v + (int)is_signed ) {}
    Literal(): Literal(-1,false) {}
    static Literal from_index( int i ) { Literal l; l.value = i; return l;}

    Literal(const Literal& other) = default;

    // getter
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
    bool learned  :  1;
    uint64_t _size : 31;

    // not to be used directly, but only with allocate function
    Clause(bool l,const std::vector<Literal> &lits) :
        learned(l), _size(lits.size()) {
            std::copy(lits.begin(),lits.end(),this->begin());
            if ( learned ) get_activity() = 1.0;
        }

public:
    static Clause* allocate(const std::vector<Literal> &lits,
            bool learnt = false) {

        auto size = sizeof(Clause) + sizeof(Literal)*lits.size();
        if ( learnt ) size += sizeof(double);
        void* memory =  malloc(size);
        assert_message( memory != nullptr, "out of memory");
        return new (memory) Clause(learnt,lits);
    }

    static void deallocate(Clause* &c) {
        assert_message( c != nullptr, "freeing a null pointer");
        c->~Clause();
        free((void*)c);
        c = nullptr;
    }
    
    uint64_t size() const { return _size; }
    bool is_learned() const { return learned; }
    double &get_activity() {
        assert_message(is_learned(),"only learned clausole has activity");
        return *reinterpret_cast<double*>(end());
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

    void update_activity(double value) { get_activity()+=value; }
    void renormalize_activity(double value) { get_activity()/=value; }

    // this metod can be used to reduce the size of the literal
    void shrink( size_t new_size) {
        assert_message( new_size <= _size, "cannot increase size");
        if ( new_size == _size ) return;
        _size = new_size;
    }
};

/**
 * Binary heap class, based on the CLR description
 */
class Literal_Heap {
    using size_type = size_t;
public:
    explicit Literal_Heap(const std::vector<double> &v):
        activity(v), value(), map_position()  {}

    Literal pop_max() {
        assert_message(!value.empty(), "pop from an empty heap");

        auto max = value.front();
        value.front() = value.back();
        map_position[max.index()] = -1;
        map_position[value.front().index()] = 0;
        value.pop_back();
        heapify(0);
        return max;
    }

    void insert(const Literal& val) {
        if ( map_position[val.index()] != -1 ) return; // already inserted

        value.push_back(val);
        map_position[val.index()] = static_cast<int>(value.size()-1);
        increase_key(value.size()-1);
    }

    void update(const Literal& val) {
        if ( map_position[val.index()] != -1 )
            increase_key(static_cast<unsigned int>(map_position[val.index()]));
    }

    void set_size( unsigned int s ) {
        map_position.resize(s,-1);
    }

    void initialize() {
        value.clear();
        value.reserve(map_position.size());

        for ( int i = 0; i < static_cast<int>(map_position.size()/2); ++i ) {

            Literal l(i,false);
            value.push_back(l);
            map_position[l.index()] = value.size()-1;

            l = Literal(i,true);
            value.push_back(l);
            map_position[l.index()] = value.size()-1;

        }
        assert_message( value.size() == map_position.size(), "");

        for ( int i = value.size()/2; i >= 0; --i )
            heapify(i);

    }


private:

    const std::vector<double> &activity;
    std::vector<Literal> value;
    std::vector<int> map_position;

    // tree navigation (0 based, it's different from CLR's 1 based)
    size_type father(size_type i) { return (i-1) >> 1; }
    size_type left(size_type i)   { return (i<<1) + 1; }
    size_type right(size_type i)  { return (i+1) << 1; }

    void heapify(size_type i) {
        //std::cout <<"heapify " << i << " " << left(i) << " " << right(i) << std::endl;
        auto largest = i;
        if (left(i) < value.size()  && activity[value[largest].index()] < activity[value[left(i)].index()] )
            largest = left(i);
        if (right(i) < value.size()  && activity[value[largest].index()] < activity[value[right(i)].index()] )
            largest = right(i);

        if ( largest != i ) {
            std::swap(value[i],value[largest]);
            std::swap(map_position[value[i].index()],
                    map_position[value[largest].index()]);

            heapify(largest);
        }
    }

    void increase_key( size_type pos ) {
        while ( pos > 0 && activity[value[father(pos)].index()] < activity[value[pos].index()] ) {
            std::swap(value[pos],value[father(pos)]);
            std::swap(map_position[value[pos].index()],
                    map_position[value[father(pos)].index()]);
            pos = father(pos);
        }
    }
};

/**
 * implementation of the VSIDS heuristic for literal selection
 */
class Literal_Order {

public:
    Literal_Order( const std::vector<double> &act,
            const std::vector<literal_value> &as):
        assignment(as), order(act) {}

    Literal decision() {
        Literal l;
        do { l = order.pop_max(); } while(assignment[l.var()] != LIT_UNASIGNED);
        return l;
    }

    void increase_activity( Literal l ) {
        order.update(l);
    }

    void insert(int value) {
        order.insert(Literal(value,true ));
        order.insert(Literal(value,false));
    }

    void set_size(unsigned int s) {
        order.set_size(s);
    }

    void initialize_heap() {
        order.initialize();
    }

private:
    const std::vector<literal_value> &assignment;
    Literal_Heap order;
};

} // end namespace Satyricon

#endif
