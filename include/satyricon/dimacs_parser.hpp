#ifndef DIMACS_PARSER_HH
#define DIMACS_PARSER_HH

#include <string>
#include <vector>
#include "data_structure.hpp"
#include "sat_solver.hpp"

namespace Satyricon {

class DimacsParser
{
    public:
        DimacsParser();
        ~DimacsParser();
        static void parse_file(std::istream &in);
};

} // end namespace Satyricon

#endif
