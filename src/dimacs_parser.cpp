#include "dimacs_parser.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <exception>
#include <set>

using namespace std;

set<vector<int> > DimacsParser::parse_file( const char * file_name)
{
    int number_of_clausole, number_of_variable;
    ifstream in(file_name, ifstream::in);
    set<vector<int> > clausole;
    string line;

    if ( ! in )
        throw invalid_argument(string("unable to read file ") + file_name);

    // find first line
    while (getline(in, line)) {
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

    // read clausole
    while (getline(in, line)) {
        if (line[0] == 'c') continue;

        vector<int> c;
        std::istringstream iss(line);
        int value;

        while( !iss.eof() ) {
            if ( !(iss >> value) )
                throw domain_error("invalid simbol on clausole " + line);

            if (value == 0) break;

            if( value < -number_of_variable || value > number_of_variable)
                throw domain_error(string("invalid variable ")
                        + to_string(value));

            c.push_back(value);
        }
        clausole.insert(c);
    }

    if (clausole.size() != number_of_clausole)
        throw domain_error("wrong number of clausole");

    return clausole;
}
