#ifndef ARGUMENT_PARSER
#define ARGUMENT_PARSER

/**
 * A small, header only, argument parsing library.
 * It support three kind of item: positional, flag and option
 * positional are identified by the their position in the argument, and doesn't
 * have a short or long identifier
 * flag are only on and of, the can have single dash short identifier (eg: -h)
 * or doble dashed long identifier (eg: --help)
 * Option are like flag but they have one argument. (eg: -o <output_file>)
 * multiple argument are not supporteted.
 */
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <assert.h>

namespace Utils {

using ParsingException = std::runtime_error;

/**
 * The abstract class for flag, option and positional.
 */
class ArgObject {
public:
    ArgObject(const std::string& _n, const std::string& _d) :
        name(_n), description(_d), parsed(false) {}
    virtual ~ArgObject() {}

    bool is_parsed() { return parsed; }
    bool set_parsed(bool b) { parsed = b; }
    explicit operator bool() { return is_parsed(); }

    const std::string& get_name() const { return name; }
    const std::string& get_description() const { return description; }
    virtual const std::string get_message() = 0;

private:
    std::string name;
    std::string description;
    bool parsed;
};

/**
 * Flag doesn't take a value, they are only on or off
 */
class ArgFlag : public ArgObject {

public:
    ArgFlag(const std::string& _name, const std::string& _description,
            const std::vector<std::string>& _flags);
    std::vector<std::string> flags;
    const std::string get_message();
};

/**
 * abstract class for option and positional.
 * both of them have a specific value
 */
class ValueArgObj : public ArgObject {
public:
    ValueArgObj(const std::string& _name, const std::string& _description) :
        ArgObject(_name,_description) {}
    virtual void parse_input(const std::string& in) = 0;
};

/**
 * templatic class for argument with a type. Used by positional and option
 */
template<typename T>
class TypedArgObj : public ValueArgObj {

public:

    TypedArgObj(const std::string& _name, const std::string& _description) :
        ValueArgObj(_name,_description) {}

    const T& get_value() { return value; }
    void set_value( const T& v) { value = v; }

    void parse_input(const std::string& in) {
        std::istringstream iss(in);
        iss >> value;
        if (iss.fail()) throw ParsingException("invalid type for " + get_name());
        set_parsed(true);
    }

private:
    T value;
};

// specialization of the template for handling string with white space etc...
template<>
inline void TypedArgObj<std::string>::parse_input( const std::string& in) {
    value = in;
    set_parsed(true);
}

/**
 * Positional class. They are identified by their position in the argument, and
 * not by a dashed identifier. They have a type.
 */
template<typename T>
class ArgPositional : public TypedArgObj<T> {

public:
    ArgPositional(const std::string& _name, const std::string& _description, size_t _pos) :
        TypedArgObj<T>(_name,_description), pos(_pos) {}

    size_t get_position() { return pos; };
    void set_position(size_t p ) { pos = p; };
    const std::string get_message();

private:
    size_t pos;
};

template<typename T>
const std::string ArgPositional<T>::get_message() {
    std::string opt("    ");
    opt += "<" + this->get_name() + "> ";
    int gap;
    if ( opt.size() >= 28 ) {
        opt+="\n";
        gap = 28;
    }
    else
        gap = 28 - opt.size();

    opt += std::string(gap,' ');
    opt += this->get_description();
    opt += "\n";
    return opt;
}

/**
 * Option are identified by a dashed character or a double dashed word.
 * they have a type.
 */
template<typename T>
class ArgOption : public TypedArgObj<T> {

public:
    ArgOption(const std::string& _name, const std::string& _description,
            const std::vector<std::string>& _opts) :
        TypedArgObj<T>(_name,_description), opts(_opts) {}
    std::vector<std::string> opts;
    const std::string get_message();
};

template<typename T>
const std::string ArgOption<T>::get_message() {
    std::string opt("    ");
    for (auto o : opts) {
        if (o.size() == 1) opt += "-"  + o + " ";
        else               opt += "--" + o + " ";
    }
    opt += "<" + this->get_name() + "> ";
    int gap;
    if ( opt.size() >= 28 ) {
        opt+="\n";
        gap = 28;
    }
    else
        gap = 28 - opt.size();

    opt += std::string(gap,' ');
    opt += this->get_description();
    opt += "\n";
    return opt;
}

/**
 * the parser. It is possible to add new element to the parser with the 
 * make_{flag,positional,option} method. when the parser is set, call
 * parseCLI with the actual argument received by the main, and all the
 * argument object are initialized from the parsed value
 */
class ArgumentParser {

public:
    ArgumentParser( const std::string& _short, const std::string& _long) :
        short_text(_short), long_text(_long) {}

    ~ArgumentParser();

    template<typename T> ArgPositional<T>&  make_positional(
            const std::string& name,
            const std::string& description);

    template<typename T> ArgOption<T>& make_option(
            const std::string& name,
            const std::string& description,
            const std::vector<std::string>& opts);

    ArgFlag& make_flag(
            const std::string& name,
            const std::string& description,
            const std::vector<std::string>& flg);

    std::string get_message() const;
    void parseCLI(int argc, char* argv[]);

private:
    std::set<std::string> used_name;
    std::vector<ValueArgObj *> positionals;
    std::vector<ValueArgObj *> options;
    std::vector<ArgFlag *> flags;

    std::map<std::string,ValueArgObj *> option_mapping;
    std::map<std::string,ArgObject   *> flag_mapping;

    std::string program_name;
    const std::string short_text;
    const std::string long_text;
};

template<typename T>
ArgPositional<T>&  ArgumentParser::make_positional( const std::string& name,
        const std::string& description) {
    auto p = new ArgPositional<T>(name,description,positionals.size());
    positionals.push_back(p);
    return *p;
}

template<typename T>
ArgOption<T>& ArgumentParser::make_option( const std::string& name,
        const std::string& description, const std::vector<std::string>& opts) {
    auto o = new ArgOption<T>(name,description,opts);
    options.push_back(o);

    for ( auto i : opts ) {
        option_mapping.insert( make_pair (i, o));
        used_name.insert(i);
    }

    return *o;
}

inline std::string wrap_string(const std::string &s, int limit) {

    std::istringstream iss(s);
    std::string wrapped, line;

    while( std::getline(iss,line) ) {

        size_t start = 0;
        while ( (start+limit) < line.size() ) {
            auto offset = line.find_last_of(' ', start + limit);
            wrapped += std::string( line, start, offset-start ) + "\n";
            start = offset+1;
        }
        wrapped += std::string(line, start, std::string::npos) + "\n";
    }

    return wrapped;
}

inline ArgFlag::ArgFlag(const std::string& _name, const std::string& _description,
        const std::vector<std::string>& _flags) :
    ArgObject(_name,_description), flags(_flags) {}

inline ArgumentParser::~ArgumentParser() {
    for ( auto i : positionals ) delete i;
    for ( auto i : options  )    delete i;
    for ( auto i : flags )       delete i;

    for ( auto i : option_mapping ) i.second = nullptr;
    for ( auto i : flag_mapping )   i.second = nullptr;
}

inline ArgFlag& ArgumentParser::make_flag( const std::string& name,
        const std::string& description, const std::vector<std::string>& flg) {

    auto f = new ArgFlag(name,description,flg);
    flags.push_back(f);

    for ( auto i : flg ) {
        flag_mapping.insert( std::make_pair(i, f));
        used_name.insert(i);
    }

    return *f;
}

inline bool is_positional( const std::string &s) {
    return s.size() >= 1 && s[0] != '-';
}

inline std::string strip_line_from_beginning( const std::string &s) {
    if (s.size() == 2) return std::string(1,s[1]);
    else               return std::string(s,2);
}

inline void ArgumentParser::parseCLI(int argc, char* argv[]) {

    program_name = std::string(argv[0]); // name of the executable
    size_t pos = 0;

    for ( int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);

        if (is_positional(arg)) {
            if ( pos >= positionals.size() )
                throw ParsingException("Too many positional argument");
            positionals[pos++]->parse_input(arg);
            continue;
        }

        std::string name = strip_line_from_beginning(arg);

        if ( flag_mapping.find(name) != flag_mapping.end() )
            flag_mapping[name]->set_parsed(true);

        else if ( option_mapping.find(name)  != option_mapping.end()) {

            if ( ++i == argc )
                throw ParsingException("invalid use of option");
            option_mapping[name]->parse_input(std::string(argv[i]));

        }
        else throw ParsingException(name + " is not a flag or option");
    }
}

inline const std::string ArgFlag::get_message() {

    std::string opt("    ");

    for (auto o : flags) {
        if (o.size() == 1) opt += "-"  + o + " ";
        else               opt += "--" + o + " ";
    }

    int gap;
    if ( opt.size() >= 28 ) {
        opt+="\n";
        gap = 28;
    }
    else
        gap = 28 - opt.size();

    opt += std::string(gap,' ');
    opt += get_description();
    opt += "\n";
    return opt;
}

inline std::string ArgumentParser::get_message() const {

    std::string message("Usage: "+program_name);
    if  ( !options.empty() || !flags.empty() ) message+=" {OPTIONS}";
    for (auto i : positionals)                 message+=" ["+i->get_name()+"]";

    message += "\n" + wrap_string(short_text,80);
    message += "\nOptions:\n";

    for (auto i : positionals) { message += i->get_message(); }
    for (auto i : options)     { message += i->get_message(); }
    for (auto i : flags)       { message += i->get_message(); }

    message+= "\n";
    message += wrap_string(long_text,80);
    return message;
}

inline std::ostream & operator<<(std::ostream &os, Utils::ArgumentParser const &a) {
    return os << a.get_message();
}

} // end namespace Utils

#endif
