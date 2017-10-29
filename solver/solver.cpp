#include <iostream>
#include <exception>
#include <cstdlib>
#include "dimacs_parser.hpp"

using namespace std;

int main(int argc, char* argv[])
{

    if (argc < 2 ) return EXIT_FAILURE;

    try {
        auto s = DimacsParser::parse_file(argv[1]);

        for (auto i : s )
        {
            for (auto j : i)
                cout << j << " ";
            cout << endl;
        }
    }
    catch (const exception& e) {
        cout << "ERROR: " << e.what() << endl;
    }

    return EXIT_SUCCESS;
}
