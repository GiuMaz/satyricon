/**
 * SAT solver.
 */
#include <iostream>
#include <iomanip>
#include <exception>
#include <cstdlib>
#include <chrono>
#include "dimacs_parser.hpp"
#include "sat_solver.hpp"
#include "ArgumentParser.hpp"
#include "log.hpp"
#include "literal.hpp"

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
    auto& verbose = parser.make_flag("verbose",
            "print the resolution process step by step.\n"
            "WARNING: can be really expansive",{"v","verbose"});

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

    Satyricon::SATSolver solver;
    Utils::Log log(verbose ? Utils::LOG_VERBOSE : Utils::LOG_NORMAL);
    solver.set_log(log);

    auto start = chrono::steady_clock::now();

    // parsing file
    try {
        bool conflict = Satyricon::parse_file(solver,cin);

        auto init_time = chrono::steady_clock::now();
        chrono::duration<double> elapsed = init_time - start;
        log.normal << "read file and initialized solver in: " << elapsed.count() << "s\n";
        if ( conflict ) {
            log.verbose << "found a conflict during the solver construction\n";
            log.normal << "insoddisfacibile\n";
            return 0;
        }
    }
    catch (const exception& e) {
        cout << "Error parsing the file: " << e.what() << endl;
        return 1;
    }

    // set option in solver
    // ...

    auto satisfiable = solver.solve();

    auto end_time = chrono::steady_clock::now();
    chrono::duration<double> elapsed = end_time - start;
    log.normal << "completed in: " << fixed << setprecision(2) << elapsed.count() << "s\n";

    log.normal << (satisfiable ? "SATISFIABLE" : "UNSATISFIABLE") << endl;
    if ( print_proof && satisfiable == true )
        cout << "Model: " << endl << solver.string_model() << endl;
    if ( print_proof && satisfiable == false )
        cout << "Counterproof: " << endl << solver.string_conterproof() << endl;

    return 0;
}

