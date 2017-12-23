#include "dimacs_parser.hpp"
#include "sat_solver.hpp"
#include "literal.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <exception>

using namespace std;
using namespace Satyricon;

bool Satyricon::parse_file(SATSolver& solver, istream & is)
{
    int number_of_clausole, number_of_variable;
    string line;

    // find header line
    while (getline(is, line)) {
        if (line[0] == 'c') continue;

        std::istringstream iss(line);
        string p, cnf, other;

        if ( !(iss >> p >> cnf >> number_of_variable >> number_of_clausole)
                || (iss >> other) || p != "p" || cnf != "cnf")
            throw domain_error(
                    "expected a 'p cnf NUMBER_OF_VARIABLE NUMBER_OF_CLAUSOLE'"
                    " as first line");
        break;
    }
    solver.set_number_of_variable(number_of_variable);

    int read_clausole = 0;
    // read clausoles
    while (getline(is, line) && read_clausole < number_of_clausole ) {
        if ( line.size() == 0 || line[0] == 'c') continue;

        std::vector<Literal> c;
        std::istringstream iss(line);
        int value;

        while( !iss.eof() ) {
            if ( !(iss >> value) )
                throw domain_error("invalid simbol on clausole " + line);

            if (value == 0) break;

            if( value < -number_of_variable || value > number_of_variable)
                throw domain_error(string("invalid variable ")
                        + to_string(value));

            c.push_back( Literal(abs(value)-1, value < 0) );
        }
        read_clausole++;
        bool conflict = solver.add_clause(c);
        if ( conflict ) return true; // found a conflict
    }
    return false; // no conflict
}

