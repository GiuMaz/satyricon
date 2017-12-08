#include "sat_solver.hpp"
#include "data_structure.hpp"

using namespace std;
using namespace Satyricon;

Solution SATSolver::solve(Formula &f) {
    Assignment a(f.number_of_variable,LIT_UNASIGNED);

    //preprocessor(f);

    //if( UnitPropagation(f,a) == CONFILICT)
        //return false;
    /*
    // iteration
    int dl = 0;
    while ( ! AllVariablesAssigned(f,a)) {

        int x = PickBranchingVariableX(f,a); // variable --> VSID
        int v = PickBranchingVariableV(f,a); // value of x
        ++dl;
        a.insert(make_pair(x,v)); // change the assignment
        // propagation
        if (UnitPropagation(f,a) == CONFILICT) { // if conflict
            int b = ConflictAnlaysis(f,a); // learn a new clausole
            if ( b < 0 ) return false; // if is the emtpy clausole, UNSAT
            Backtrack(f,a,b); // backjumping
            dl = b;
        }
    }
    */
    return Solution(false,"");
}

