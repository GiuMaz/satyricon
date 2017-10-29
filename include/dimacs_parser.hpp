#ifndef DIMACS_PARSER_HH
#define DIMACS_PARSER_HH

#include <string>
#include <vector>
#include <set>

class DimacsParser
{
    public:
        DimacsParser();
        ~DimacsParser();
        static std::set<std::vector<int> > parse_file(const char * file_name);
};

#endif
