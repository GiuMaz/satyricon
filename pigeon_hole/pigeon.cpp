#include <iostream>
#include <fstream>
#include <sstream>
#include <exception>
#include <cstdlib>

using namespace std;

#define VARIABLE(p, h)  ( (p-1) * hole + h )

int main(int argc, char* argv[])
{
    if (argc < 2 ) return EXIT_FAILURE;

    stringstream ss(argv[1]);
    int hole;
    ss >> hole;
    int pigeon = hole + 1;


    ofstream ofs(string("pigeon_") + to_string(hole) + ".txt");

    ofs << "c pigeon hole problem with "<< hole
        << " hole and " << pigeon << " pigeon\n";

    ofs << "p cnf " <<  (hole * pigeon) << " " <<
        ( pigeon + ((hole*hole*pigeon)/2)) <<"\n";

    for ( int p = 1; p <= pigeon; ++p)
    {
        for ( int h = 1; h <= hole; ++h)
            ofs << VARIABLE(p,h) << " ";
        ofs << "0\n";
    }

    for ( int j = 1; j <= hole; ++j)
        for ( int i = 1; i <= (pigeon-1); ++i)
            for ( int h = (i+1); h <= pigeon; ++h)
                ofs << -VARIABLE(i,j) << " " << -VARIABLE(h,j) << " 0\n";
        
    return EXIT_SUCCESS;
}
