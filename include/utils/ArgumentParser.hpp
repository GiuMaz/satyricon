#ifndef ARGUMENT_PARSER
#define ARGUMENT_PARSER

/**
 * A Small header only argument parsing class.
 * It support only a limited kind of argument.
 */

#include <map>
#include <utility>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <assert.h>

namespace Utils {

class ArgumentParser {

public:

    ArgumentParser(const std::string& short_text, const std::string& long_text);
    ~ArgumentParser();

    void add_positional( const std::string& name,
            const std::string& description);

    void add_option( const std::string& name, char opt,
            const std::string& description);

    void add_flag( const std::string& name, char opt,
            const std::string& description);

    bool has(const std::string& argument);

    template<typename T>
        T get(const std::string& argument);

    std::string get_message() const;

    void parseCLI(int argc, char* argv[]);


private:

    struct Option   { char   opt; std::string name; std::string description; };
    struct Position { size_t pos; std::string name; std::string description; };
    struct Flag     { char   opt; std::string name; std::string description; };

    std::map<std::string,std::string> parsed;

    std::set<char> used_chars;
    std::map<char,std::string> char_flags;
    std::map<char,std::string> char_options;

    std::vector<Position> positionals;
    std::vector<Flag> flags;
    std::vector<Option> options;

    std::string short_text;
    std::string long_text;

};

ArgumentParser::ArgumentParser( const std::string& s, const std::string& l) :
    short_text(s), long_text(l) {
}

ArgumentParser::~ArgumentParser() {}

void ArgumentParser::add_positional( const std::string& name,
        const std::string& description) {
    positionals.push_back( { positionals.size()+1, name, description} );
}

void ArgumentParser::add_option( const std::string& name, char opt,
        const std::string& description){

    assert( used_chars.find(opt) == used_chars.end() );
    char_options.insert( make_pair (opt, name));
    options.push_back( { opt, name, description } );
}

void ArgumentParser::add_flag( const std::string& name, char opt,
        const std::string& description) {

    assert( used_chars.find(opt) == used_chars.end() );
    char_flags.insert( make_pair (opt, name));
    flags.push_back( { opt, name, description } );
}

void ArgumentParser::parseCLI(int argc, char* argv[]) {

    size_t position = 0;
    for ( int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);

        if ( arg[0] == '-' && char_flags.find(arg[1]) != char_flags.end() ) {
            parsed.insert ( std::make_pair( char_flags[arg[1]], ""));
            continue;
        }

        if ( arg[0] == '-' && char_options.find(arg[1]) != char_options.end() )
        {
            if ( ++i == argc )
                throw std::domain_error("invalid use of option");

            parsed.insert ( std::make_pair( char_options[arg[1]], argv[i]) );
            continue;
        } 

        if ( position > positionals.size() )
            throw std::domain_error("too many positional arguments");

        parsed.insert( std::make_pair(positionals[position++].name, arg));
    }
}

bool ArgumentParser::has(const std::string& argument) {
    return parsed.find(argument) != parsed.end();
}

template<typename T>
T ArgumentParser::get(const std::string& argument){
    std::istringstream iss( parsed[argument] );
    T ret;
    iss >> ret;
    assert( !iss.fail() );
    return ret;
}

std::string ArgumentParser::get_message() const{
    return short_text + "\n";
}

std::ostream & operator<<(std::ostream &os, ArgumentParser const &a) {
    std::string s = a.get_message();
    return os << s;
}

} // end namespace Utils

#endif
