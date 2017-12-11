/**
 * SAT solver.
 */
#include <iostream>
#include <exception>
#include <cstdlib>
#include "dimacs_parser.hpp"
#include "data_structure.hpp"
#include "sat_solver.hpp"
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

    auto& in = parser.make_positional<string>("input","input file");
    auto& help = parser.make_flag("help",
            "print this message and exit",{"h","help"});
    auto& print_proof = parser.make_flag("print_proof",
            "print proof",{"p","proof"});

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

    // redirect input file
    if ( in ) freopen( in.get_value().c_str(), "r", stdin);

// -----------------------------------------------------------------------------

    // SOLVER

    Satyricon::Formula formula;
    // parsing file
    try {
        formula = Satyricon::DimacsParser::parse_file(cin);
    }
    catch (const exception& e) {
        cout << "ERROR: " << e.what() << endl;
        return 1;
    }

    Satyricon::SATSolver solver(formula);

    // set option in solver
    // ...

    auto result = solver.solve();

    cout << (result.is_sat() ? "soddisfacibile" : "insoddisfacibile") << endl;
    if ( print_proof ) cout << result.get_solution();

    return 0;
}

