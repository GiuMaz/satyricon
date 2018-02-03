#ifndef SATYRICON_SOLVER_HPP
#define SATYRICON_SOLVER_HPP

#include <queue>
#include <vector>
#include <memory>
#include <unordered_map>
#include "literal.hpp"
#include "log.hpp"
#include "vsids.hpp"

namespace Satyricon {

class Clause;

typedef int var;

/**
 * SAT Solver.
 * This class is used to solve a SAT problem instance.
 */
class SATSolver {

public:

    SATSolver();
    ~SATSolver();

    // TODO: for minisat compatibility, and for problem creation as a library
    //var newVar(); // get a new variable
    
    // Set the number of variable that can be used in the sat problem.
    // every possible variable is an atom that can be negated or not.
    void set_number_of_variable(unsigned int n);

    // Add a new clause to the problem. The clause is a list of literal.
    bool add_clause(std::vector<Literal>& c);

    // Solve the problem instance, return true iff the problem is satisfiable
    bool solve();

    // Set the logger
    void set_log( Utils::Log l );

    // if the formula is satisfiable, return the model
    const std::vector<int>& get_model();

    // return a printable version of the model.
    std::string string_model();

    // decay factor for clause activity
    void set_clause_decay(double clause_decay_factor);

    // decay factor for literal activity
    void set_literal_decay(double literal_decay_factor);

    // enable or diable preprocessing of clauses
    void set_preprocessing(bool p);

    // enable or disable search restart
    void set_restart(bool p);

    // enable or disable deletion stratergy of learned clause
    void set_deletion(bool p);

    // multiply the lenght of each restart sequence
    void set_restarting_multiplier(unsigned int b);

    // define the starting maximum for learned clause.
    // is a multiple of the number of clause in the original problem
    void set_learning_multiplier( double value );

    // if the learned clause reach the learning limit, half the clause are
    // eliminated and the learn limit is increased by this factor
    void set_learning_increase( double value );

private:
    friend class Clause;

    // forward declaration of support class Clause

    // handfull type declaration
    using ClausePtr = Clause*;
    using WatchMap = std::vector<std::vector<ClausePtr> >;
    using SubsumptionMap = std::vector<std::vector<ClausePtr> >;

    // print the search status
    void print_status(unsigned int conflict, unsigned int restart,
            unsigned int learn_limit);

    // build proof of satisfiability
    void build_sat_proof();

    // learn the conflict clause
    //bool learn_clause();
    void learn_clause(std::vector<Literal> & lits);

    bool new_clause(std::vector<Literal> & lits, bool learnt, ClausePtr &c_ref);

    void remove_from_vect( std::vector<ClausePtr> &v, ClausePtr c );

    void remove_clause( ClausePtr c );

    // get value of a literal
    literal_value get_asigned_value(const Literal & l) const;

    // assign a literal l with antecedent c (nullptr for decided)
    bool assign(Literal l, ClausePtr c);

    // propage the effect of the previous assignment
    ClausePtr propagation();

    // analyze a conflict clause and create a new clause to be learned
    // and a proper backtrack level
    int conflict_analysis();
    void conflict_analysis(ClausePtr conflict,
            std::vector<Literal> &out_learnt, int &out_btlevel);

    // Preprocess the set of clause
    void preprocessing();
    bool subset(const Clause& inner,const Clause& outer);

    // interval before restart
    unsigned int next_restart_interval();
    unsigned int new_restart_threshold();

    // decide a literal
    bool assume( Literal p );

    // reduce learned clause. The clauses are sorted by activity,
    // and the lower half are removed execept of clauses that are the
    // antecedent of an assigned literal.
    void reduce_learned();

    // decay the activity of clause. This is an O(1) operation
    void clause_activity_decay();

    // clause collections
    std::vector<ClausePtr > clauses;
    std::vector<ClausePtr > learned;
    unsigned int number_of_variable;

    // watch list, used for propagation
    WatchMap watch_list;

    // assigned values
    std::vector<literal_value> values;
    std::vector<int> decision_levels;
    std::vector<ClausePtr > antecedents;

    // after an assignment is performed, both for decision or unit propagation,
    // the effect of the assignment on the other clause must be propagated
    std::queue<Literal> propagation_queue;

    // VSIDS
    VSIDS_Info vsids;

    // trial of assignment
    std::vector<Literal> trail;
    std::vector<int>  trail_limit;

    // current level of research
    inline int current_level() const;
    void undo_one();
    void cancel();
    void cancel_until( int level );

    //number of assinged variable
    size_t number_of_assigned_variable() const;

    // log file
    Utils::Log log;

    // if the clause is sat, this vector contein a model for the solution
    std::vector<int> model;

    // information needed in the preprocessing step
    SubsumptionMap subsumption;

    // enable or disable feature
    bool enable_preprocessing;
    bool enable_restart;
    bool enable_deletion;

    // values for luby sequence, used for restart policy
    size_t luby_k    = 1;
    size_t luby_next = 1;
    std::vector<unsigned int> luby_memoization {};

    // restarting 
    unsigned int restart_interval_multiplier;
    unsigned int restart_threshold;

    // clause deletion policy
    double clause_activity_update;
    double clause_decay_factor;

    // clause deletion policy
    double initial_learn_mult;
    double percentual_learn_increase;
};

} // end namespace Satyricon

#endif
