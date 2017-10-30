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

using namespace std;

void print_usage()
{
    cout << "pigeon_hole [-h] [-o output_file] N\n"
        "Generate pigeon hole problem with N hole and N+1 pigeon.\n"
        "N must be > 0.\n"
        "Use dimacs cnf format, the result is save in output_file (if"
        " specified) or inside 'pigeon_<N>.cnf' otherwhise\n\n"
        "\t-h      -- print this message and exit\n\n"
        "\t-o file -- specify output file name\n\n";
}

void usage_error()
{
    print_usage();
    exit(1);
}

int main(int argc, char* argv[])
{
    // Argument parsiong
    if (argc < 2) usage_error();

    string out_file_name;
    int hole = 0, pigeon = 0;

    for (int i = 1; i < argc; i++) {

        string param(argv[i]);

        if (param == "-h") { 
            print_usage();
            return EXIT_SUCCESS;
        }

        if (param == "-o")
        {
            if ( out_file_name != "" &&  ++i >= argc) usage_error();

            out_file_name = string(argv[i]);
            continue;
        }

        stringstream ss(argv[i]);
        if (!(ss >> hole)) usage_error();
        pigeon = hole + 1;
    }

    if (hole <= 0) usage_error();

    if (out_file_name == "")
        out_file_name = (string("pigeon_") + to_string(hole) + ".cnf");

// -----------------------------------------------------------------------------

    // PROBLEM GENERATION
    ofstream ofs(out_file_name);

    // lambda function used for variable number generation
    auto variable = [hole](int p, int h) {  return (p-1) * hole + h; };

    // header
    ofs << "c pigeon hole problem with "<< hole
        << " hole and " << pigeon << " pigeon\n";
    ofs << "p cnf " <<  (hole * pigeon) << " " <<
        ( pigeon + ((hole*hole*pigeon)/2)) <<"\n";

    // clausoles for have a hole for every pigeon
    for ( int p = 1; p <= pigeon; ++p) {
        for ( int h = 1; h <= hole; ++h)
            ofs << variable(p,h) << " ";
        ofs << "0\n";
    }

    // clausoles for having a single pigeon for every hole
    for ( int j = 1; j <= hole; ++j)
        for ( int i = 1; i <= (pigeon-1); ++i)
            for ( int h = (i+1); h <= pigeon; ++h)
                ofs << -variable(i,j) << " " << -variable(h,j) << " 0\n";
        
    return EXIT_SUCCESS;
}
