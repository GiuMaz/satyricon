#include <iostream>
#include <csignal>
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

// description of program, both for documenation and help message
std::string program_description =
"This program can solve propositional logic problems written in conjunctive "
"normal form. It is possible to specify a file with the problem's constraints "
"written in DIMACS format.\n"
"(DIMACS: http://www.satcompetition.org/2009/format-benchmarks2009.html).\n"
"If no file is specified, the program reads from the standard input, the input "
"must follow the DIMACS format as previously specified."
"This solver is based on the CDCL resolution scheme, so after a conflict it "
"learn a new clause and try to use it to improve the searching process. "
"This program use the VSIDS heuristic for the selection of new decision "
"literals based on the 'activity' of a literal. A really similar mechanism "
"is used for evaluate the activity of a learned clause, and clauses with low "
"activity are periodically removed from the problem.\n"
"The program periodically restart the searching process keeping all the "
"learned information.\n"
"A preprocessing step is applied to the problem before the resolution, in "
"which a clause that can be subsumed by a more general one are eliminated from "
"the formula.\n"
"It is possible to change and/or disable all this features from the command "
"line interace, as described in the 'Options' section.\n"
"If requested, the program can build a proof of the "
"satisiability/unsatisfiablity results. If the problem is SATISFIABLE, a model "
"are given, with a possible asignment of all the literals that can satisfy all "
"the clauses. If the problem is UNSATISFIABLE the program show how is possible "
"to build the empty clause with a resolution process of the original clauses "
"of the problem."
;

std::chrono::time_point<std::chrono::steady_clock> start;

void signalHandler( int signum ) {
   cout << "Interrupt signal (" << signum << ") received.\n";

    auto end_time = chrono::steady_clock::now();
    chrono::duration<double> elapsed = end_time - start;
    cout << "stopped after: " << fixed << setprecision(2) <<
        elapsed.count() << "s\n";

   cout << "UNKOWN\n";
   exit(1);  
}

/**
 * SAT solver from CLI
 */
int main(int argc, char* argv[])
{
    // SIGNAL HANDLING
    signal(SIGINT, signalHandler); 
    signal(SIGTERM, signalHandler); 
    signal(SIGKILL, signalHandler); 

    // ARGUMENT PARSING

    // parsing set up
    ArgumentParser parser(
            "SAT solver for propositional logic",
            program_description);

    // input file (if not specified, read from stdin
    auto& in = parser.make_positional<string>("input",
            "input file (in DIMACS format). If not specified, use stdin" );

    // print help
    auto& help = parser.make_flag("help",
            "print this message and exit",{"h","help"});
    // verbose output
    auto& verbose = parser.make_flag("verbose",
            "print the resolution process step by step.\n"
            "WARNING: can be really expansive",{"v","verbose"});

    // print proof
    auto& print_sat_proof = parser.make_flag("sat_proof",
            "print the model if the formula is satisfiable",{"s","proof-sat"});
    auto& print_unsat_proof = parser.make_flag("unsat_proof",
            "print proof if the formula is unsatisfiable",{"u","proof-unsat"});
    auto& print_proof = parser.make_flag("print_proof",
            "print proof (both for sat and unsat)",{"p","proof"});

    // disable feature (for testing purpose)
    auto& no_preproc = parser.make_flag("no_preprocessing",
            "disable preprocessing of clause",{"no-preprocessing"});
    auto& no_restart = parser.make_flag("no_restart",
            "disable search restart",{"no-restart"});
    auto& no_deletion = parser.make_flag("no_deletion",
            "disable deletion of learned clauses",{"no-deletion"});

    // decay policy
    double decay_literal_factor = 0.95, decay_clauses_factor = 0.999;
    auto& clause_decay = parser.make_option<double>("clause decay",
            "decay factor for activity of clauses.\nSould be "
            "0 < c-decay ≤ 1.0 (default " +
            std::to_string(decay_clauses_factor)+")",{"c-decay"});
    auto& literal_decay = parser.make_option<double>("literal decay",
            "decay factor for activity of literal.\nShould be "
            "0 < l-decay ≤ 1.0 (defualt "+
            std::to_string(decay_literal_factor)+")",{"l-decay"});

    // restarting policy
    unsigned int restart_interval_multiplier = 10;
    auto& restart_mult = parser.make_option<int>("restart multiplier",
            "restarting sequence multiplicator (default "+
            to_string(restart_interval_multiplier)+")", {"b","restart-mult"});

    // clause deletion policy
    double initial_learn_mult = 2.0, percentual_learn_increase = 50.0;
    auto& learn_mult = parser.make_option<double>("learn multiplier",
            "initial learn limit, is a multiple of the clauses in "
            " the formula (default "+ to_string(initial_learn_mult)+"x)",
            {"l","learn-mult"});
    auto& learn_increase = parser.make_option<double>("learn increase",
            "when the learn limit is reached, some learned clauses are "
            "eliminated and the learn limit is increased by this percentage "
            "(default "+ to_string(percentual_learn_increase)+"%)",
            {"i","learn-increase"});

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
    if ( clause_decay ) {
        double decay = clause_decay.get_value();
        if ( decay <= 0.0 || decay > 1.0 ) {
            cout << "ERROR: should be 0 < c-decay ≤ 1.0\n" << parser;
            exit(1);
        }
        decay_clauses_factor = decay;
    }
    if ( literal_decay ) {
        double decay = literal_decay.get_value();
        if ( decay <= 0.0 || decay > 1.0 ) {
            cout << "ERROR: should be 0 < l-decay ≤ 1.0\n" << parser;
            exit(1);
        }
        decay_literal_factor = decay;
    }

    // restarting multiplier value
    if ( restart_mult ) { 
        if ( restart_mult.get_value() < 1.0 ) {
            cout << "ERROR: should be restart-mult ≥ 1.0\n" << parser;
            exit(1);
        }
        restart_interval_multiplier = restart_mult.get_value();
    }

    // clause deletion
    if (  learn_mult ) {
        if ( learn_mult.get_value() <= 0.0 ) {
            cout << "ERROR: should be learn-mult > 0.0\n" << parser;
            exit(1);
        }
        initial_learn_mult = learn_mult.get_value();
    }
    if (  learn_increase ) {
        if ( learn_increase.get_value() < 0.0 ) {
            cout << "ERROR: should be learn-mult ≥ 0.0\n" << parser;
            exit(1);
        }
        percentual_learn_increase = learn_increase.get_value();
    }

    // print proof
    bool print_sat   = print_sat_proof   || print_proof;
    bool print_unsat = print_unsat_proof || print_proof;

// -----------------------------------------------------------------------------

    // SOLVER

    Satyricon::SATSolver solver;
    Utils::Log log(verbose ? Utils::LOG_VERBOSE : Utils::LOG_NORMAL);
    solver.set_log(log);

    start = chrono::steady_clock::now();

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

    // disable features
    if ( no_preproc  ) solver.set_preprocessing(false);
    if ( no_restart  ) solver.set_restart(false);
    if ( no_deletion ) solver.set_deletion(false);
    
    // decaying factor
    solver.set_clause_decay(decay_clauses_factor);
    solver.set_literal_decay(decay_literal_factor);

    // clause deletion
    solver.set_learning_multiplier( initial_learn_mult );
    solver.set_learning_increase( percentual_learn_increase );

    // restarting policy
    solver.set_restarting_multiplier(restart_interval_multiplier);

    // solve the formula
    bool satisfiable = solver.solve();

    // print exec time
    auto end_time = chrono::steady_clock::now();
    chrono::duration<double> elapsed = end_time - start;
    log.normal << "completed in: " << fixed << setprecision(2) <<
        elapsed.count() << "s\n";

    // print result
    log.normal << (satisfiable ? "SATISFIABLE" : "UNSATISFIABLE") << endl;
    if ( print_sat && satisfiable == true )
        cout << "Model: " << endl << solver.string_model() << endl;
    if ( print_unsat && satisfiable == false )
        cout << "Proof: " << endl << solver.unsat_proof();

    return 0;
}

