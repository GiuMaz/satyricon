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
    const string solution("solution"), help("help"), in("input");
    ArgumentParser parser(
            "SAT solver for propositional logic",
            " --- write long description --- "
            );

    parser.add_positional<string>(in,"input file");
    parser.add_flag(help,{"h","help"},"print this message and exit");
    parser.add_flag(solution,{"s","solution"},"give proof");

    // parsing argument
    try {
        parser.parseCLI(argc,argv);
    }
    catch (Utils::ParsingException &e) {
        cout << e.what() << endl;
        cout << parser;
        return 1;
    }

    if (parser.has(help)) {
        cout << parser;
        return 0;
    }

    bool print_solution = parser.has(solution);

    // redirect input file
    if ( parser.has(in)) freopen(parser.get<string>(in).c_str(),"r",stdin);

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
