/**
 * SAT solver.
 */
#include <iostream>
#include <exception>
#include <cstdlib>
#include "dimacs_parser.hpp"
#include "ArgumentParser.hpp"

using namespace std;
using Utils::ArgumentParser;

int main(int argc, char* argv[])
{
    // ARGUMENT PARSING

    // parsing set up
    ArgumentParser parser(
            "SAT solver for propositional logic",
            " --- write long description --- "
            );

    auto& in       = parser.make_positional<string>("input","input file");
    auto& help     = parser.make_flag("help","print this message and exit",{"h","help"});
    auto& solution = parser.make_flag("solution","give proof",{"s","solution"});

    // parsing argument
    try {
        parser.parseCLI(argc,argv);
    }
    catch (Utils::ParsingException &e) {
        cout << e.what() << endl;
        cout << parser;
        return 1;
    }

    if ( help ) {
        cout << parser;
        return 0;
    }

    bool print_solution = solution ? true : false;

    // redirect input file
    if ( in ) freopen( in.get_value().c_str(), "r", stdin);

    // parsing file
    try {
        auto s = DimacsParser::parse_file(cin);

        for (auto i : s ) {
            for (auto j : i) cout << j << " ";
            cout << endl;
        }
    }
    catch (const exception& e) {
        cout << "ERROR: " << e.what() << endl;
    }

    return EXIT_SUCCESS;
}
