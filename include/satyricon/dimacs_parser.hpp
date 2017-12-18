#ifndef DIMACS_PARSER_HH
#define DIMACS_PARSER_HH

#include <string>
#include <vector>
#include "sat_solver.hpp"

namespace Satyricon {

// initialize a solver with all the clause in a DIMACS file
bool parse_file( SATSolver& solver, std::istream &in);

} // end namespace Satyricon

#endif
