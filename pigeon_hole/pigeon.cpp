/**
 * Program used to generate dimacs cnf file that rappresent the pigeon hole 
 * problem,  in whit N + 1 pigeon want to fit inside only N hole. This problem
 * is always not satisaible.
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <exception>
#include <cstdlib>
#include "ArgumentParser.hpp"

using namespace std;
using Utils::ArgumentParser;

int main(int argc, char* argv[])
{
    // ARGUMENT PARSING

    // parsing set up
    const string N("N"), of("output_file"),help("help");
    ArgumentParser parser(
            "Generate pigeon hole problem with N hole and N+1 pigeon.",
            "This program generate instance of the notorius pigeon hole problem"
            ". The problem generated are always not satisfiable.\n" 
            "Generate file in dimacs cnf format. If the output file name is"
            " not specified use directly the standard output"
            );
    parser.add_positional<int>(N,"the number of holes in the problem");
    parser.add_option<std::string>(of,{"o"},"name of output file");
    parser.add_flag(help,{"h","help"},"print this message and exit");

    // parsing
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

    if ( ! parser.has(N) ) {
        cout << "Number of hole N is required\n" << parser;
        return 1;
    }

    int hole = parser.get<int>(N);
    int pigeon = hole + 1;

    // if specified, change output file
    if (parser.has(of)) freopen(parser.get<string>(of).c_str(),"w",stdout);

// -----------------------------------------------------------------------------

    // PROBLEM GENERATION
    // lambda function used for variable number generation
    auto variable = [hole](int p, int h) {  return (p-1) * hole + h; };

    // header
    cout << "c pigeon hole problem with "<< hole
        << " hole and " << pigeon << " pigeon\n";
    cout << "p cnf " <<  (hole * pigeon) << " " <<
        ( pigeon + ((hole*hole*pigeon)/2)) <<"\n";

    // clausoles for have a hole for every pigeon
    for ( int p = 1; p <= pigeon; ++p) {
        for ( int h = 1; h <= hole; ++h)
            cout << variable(p,h) << " ";
        cout << "0\n";
    }

    // clausoles for single pigeon in every hole
    for ( int j = 1; j <= hole; ++j)
        for ( int i = 1; i <= (pigeon-1); ++i)
            for ( int h = (i+1); h <= pigeon; ++h)
                cout << -variable(i,j) << " " << -variable(h,j) << " 0\n";
        
    return EXIT_SUCCESS;
}
