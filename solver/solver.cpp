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

/**
 * SAT solver from CLI
 */
int main(int argc, char* argv[])
{
    // ARGUMENT PARSING

    // parsing set up
    ArgumentParser parser(
            "SAT solver for propositional logic",
            " --- write long description --- "
            );

    // build options for command line interface usage
    auto& in = parser.make_positional<string>("input","input file");
    auto& help = parser.make_flag("help",
            "print this message and exit",{"h","help"});

    auto& print_sat_proof = parser.make_flag("sat_proof",
            "print the model if the formula is satisfiable",{"s","print-sat"});
    auto& print_unsat_proof = parser.make_flag("unsat_proof",
            "print proof when the formula is unsatisfiable",{"u","print-unsat"});
    auto& print_proof = parser.make_flag("print_proof",
            "print proof (both for sat and unsat)",{"p","proof"});

    auto& verbose = parser.make_flag("verbose",
            "print the resolution process step by step.\n"
            "WARNING: can be really expansive",{"v","verbose"});
    auto& no_preproc = parser.make_flag("no_preprocessing",
            "disable preprocessing of clause",{"no-preprocessing"});

    double decay_literal_factor = 1.05, decay_clauses_factor = 1.001;
    auto& clause_decay = parser.make_option<float>("clause decay",
            "decay factor for activity of clauses (default: " +
            std::to_string(decay_clauses_factor)+")",{"c-decay"});
    auto& literal_decay = parser.make_option<float>("literal decay",
            "decay factor for activity of literal (defualt: "+
            std::to_string(decay_literal_factor)+")",{"l-decay"});

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

    // decay values
    if ( clause_decay )  decay_clauses_factor = clause_decay.get_value();
    if ( literal_decay ) decay_literal_factor = literal_decay.get_value();

    // print proof
    bool print_sat   = print_sat_proof   || print_proof;
    bool print_unsat = print_unsat_proof || print_proof;

// -----------------------------------------------------------------------------

    // SOLVER

    Satyricon::SATSolver solver;
    Utils::Log log(verbose ? Utils::LOG_VERBOSE : Utils::LOG_NORMAL);
    solver.set_log(log);

    auto start = chrono::steady_clock::now();

    // parsing file
    try {
        bool conflict = Satyricon::parse_file(solver,cin);

        // get initilization time
        auto init_time = chrono::steady_clock::now();
        chrono::duration<double> elapsed = init_time - start;
        log.normal << "read file and initialized solver in: " <<
            fixed << setprecision(2) << elapsed.count() << "s\n";

        // if found a conflict at level zero, report the conflict
        if ( conflict ) {
            log.verbose << "found a conflict during solver construction\n";
            log.normal << "UNSATISFIABLE" << endl;
            return 0;
        }
    }
    catch (const exception& e) {
        cout << "Error parsing the file: " << e.what() << endl;
        return 1;
    }

    // set options in solver
    // enable preprocessing
    if ( no_preproc ) solver.set_preprocessing(false);
    
    // decaying factor
    solver.set_clause_decay(decay_clauses_factor);
    solver.set_literal_decay(decay_literal_factor);

    auto satisfiable = solver.solve();

    auto end_time = chrono::steady_clock::now();
    chrono::duration<double> elapsed = end_time - start;
    log.normal << "completed in: " << fixed << setprecision(2) <<
        elapsed.count() << "s\n";

    log.normal << (satisfiable ? "SATISFIABLE" : "UNSATISFIABLE") << endl;
    if ( print_sat && satisfiable == true )
        cout << "Model: " << endl << solver.string_model() << endl;
    if ( print_unsat && satisfiable == false )
        cout << "proof: " << endl << solver.string_conterproof();

    return 0;
}

