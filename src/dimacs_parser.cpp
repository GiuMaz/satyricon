#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "dimacs_parser.hpp"
#include "sat_solver.hpp"
#include "solver_types.hpp"

//using namespace std;
//using namespace Satyricon;

bool Satyricon::parse_file(SATSolver& solver, std::istream & is)
{
    unsigned int number_of_clausole = 0, number_of_variable = 0;
    std::string line;

    // find header line
    while (getline(is, line)) {
        if (line[0] == 'c') continue;

        std::istringstream iss(line);
        std::string p, cnf, other;

        if ( !(iss >> p >> cnf >> number_of_variable >> number_of_clausole)
                || (iss >> other) || p != "p" || cnf != "cnf")
            throw std::domain_error(
                    "expected a 'p cnf NUMBER_OF_VARIABLE NUMBER_OF_CLAUSOLE'"
                    " as first line");
        break;
    }
    solver.set_number_of_variable(number_of_variable);

    unsigned int read_clausole = 0;
    // read clausoles

    std::vector<Literal> c;
    int value;

    while (getline(is, line) && read_clausole < number_of_clausole ) {
        if ( line.size() == 0 || line[0] == 'c') continue;

        std::istringstream iss(line);

        while( !iss.eof() ) {
            if ( !(iss >> value) )
                throw std::domain_error("invalid simbol on clausole " + line);
            if(static_cast<unsigned int>(abs(value)) > number_of_variable)
                throw std::domain_error(std::string("invalid variable ")+std::to_string(value));

            if (value == 0) {
                read_clausole++;
                bool conflict = solver.add_clause(c);
                c.clear();
                if ( conflict ) return true; // found a conflict
            }
            else {
                c.push_back( Literal(abs(value)-1, value < 0) );
            }
        }
    }

    return false; // no conflict
}

