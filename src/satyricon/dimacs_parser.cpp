#include "dimacs_parser.hpp"
#include "data_structure.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <exception>

using namespace std;
using namespace Satyricon;

Formula DimacsParser::parse_file( istream & is)
{
    Satyricon::Formula f;
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
    f.number_of_clausole = number_of_clausole;
    f.number_of_variable = number_of_variable;

    // read clausoles
    while (getline(is, line)) {
        if ( line.size() == 0 || line[0] == 'c') continue;

        Clause c;
        std::istringstream iss(line);
        int value;

        while( !iss.eof() ) {
            if ( !(iss >> value) )
                throw domain_error("invalid simbol on clausole " + line);

            if (value == 0) break;

            if( value < -number_of_variable || value > number_of_variable)
                throw domain_error(string("invalid variable ")
                        + to_string(value));

            c.literals.push_back( Literal(abs(value-1), value < 0) );
        }
        f.clausoles.push_back(c);
    }

    if (f.clausoles.size() != number_of_clausole)
        throw domain_error("wrong number of clausoles");

    return f;
}
