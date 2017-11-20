#ifndef ARGUMENT_PARSER
#define ARGUMENT_PARSER

/**
 * A Small header only argument parsing library.
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

using ParsingException = std::runtime_error;

class ArgumentParser {

public:
    ArgumentParser(const std::string& short_text, const std::string& long_text);

    ~ArgumentParser();

    template<typename T>
        void add_positional(const std::string& name,
                const std::string& description);

    template<typename T>
        void add_option(const std::string& name,
                const std::vector<std::string>& opts,
                const std::string& description);

    void add_flag(const std::string& name,
            const std::vector<std::string>& flags,
            const std::string& description);

    bool has(const std::string& argument) const;

    template<typename T>
        T get(const std::string& argument) const;

    std::string get_message() const;

    void parseCLI(int argc, char* argv[]);

private:

    struct Flag;
    struct Position;
    template<typename T> struct TypedPosition;
    struct Option;
    template<typename T> struct TypedOption;

    std::string wrap_string( const std::string & s, int limit) const;

    std::map<const std::string, const std::string> parsed;
    std::set<std::string> used_name;

    std::vector<Position *> positionals;
    std::vector<Option *> options;
    std::vector<Flag *> flags;

    std::map<std::string,Option *> option_mapping;
    std::map<std::string,Flag *> flag_mapping;

    std::string program_name;
    const std::string short_text;
    const std::string long_text;
};

#include "ArgumentParser.i.hpp"

} // end namespace Utils

#endif
