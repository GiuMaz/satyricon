struct ArgumentParser::Flag {
    std::vector<std::string> flags;
    std::string name;
    std::string description;
};

struct ArgumentParser::Position {
    size_t pos;
    std::string name;
    std::string description;
    virtual bool validate( std::string ) = 0;
};

struct ArgumentParser::Option {
    std::vector<std::string> opts;
    std::string name;
    std::string description;
    virtual bool validate( std::string ) = 0;
};

template<typename T>
struct ArgumentParser::TypedOption : ArgumentParser::Option  {
    bool validate( std::string s ) {
        T val;
        std::istringstream iss(s);
        iss >> val;
        return !iss.fail();
    };
};

template<typename T>
struct ArgumentParser::TypedPosition : ArgumentParser::Position  {
    bool validate( std::string s ) {
        T val;
        std::istringstream iss(s);
        iss >> val;
        return !iss.fail();
    };
};

std::string ArgumentParser::wrap_string(const std::string &s, int limit) const {

    std::istringstream iss(s);
    std::string wrapped, line;

    while( getline(iss,line) ) {

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

ArgumentParser::ArgumentParser( const std::string& s, const std::string& l) :
    short_text(s), long_text(l) {}

ArgumentParser::~ArgumentParser() {

    for ( auto i : positionals ) delete i;
    for ( auto i : options  )    delete i;
    for ( auto i : flags )       delete i;

    for ( auto i : option_mapping ) i.second = nullptr;
    for ( auto i : flag_mapping )   i.second = nullptr;
}

template<typename T>
void ArgumentParser::add_positional( const std::string& name,
        const std::string& description) {

    auto t = new TypedPosition<T>();
    t->pos = positionals.size() + 1;
    t->name = name;
    t->description = description;

    positionals.push_back( t );
}

template<typename T>
void ArgumentParser::add_option( const std::string& name,
        const std::vector<std::string>& opts,
        const std::string& description){

    //assert( used_chars.find(opt) == used_chars.end() );

    auto t = new TypedOption<T>();
    t->opts = opts;
    t->name = name;
    t->description = description;
    options.push_back( t );

    for ( auto i : opts )
    {
        option_mapping.insert( make_pair (i, t));
        used_name.insert(i);
    }
}

void ArgumentParser::add_flag( const std::string& name,
        const std::vector<std::string>& fl,
        const std::string& description) {

    //assert( used_chars.find(opt) == used_chars.end() );

    auto t = new Flag();
    t->flags = fl;
    t->name = name;
    t->description = description;
    flags.push_back( t );

    for ( auto i : fl )
    {
        flag_mapping.insert( make_pair (i, t));
        used_name.insert(i);
    }
}


void ArgumentParser::parseCLI(int argc, char* argv[]) {

    size_t position = 0;
    program_name = std::string(argv[0]);

    for ( int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        auto iter = arg.begin();

        if ( *iter == '-' ) {
            iter++;

            if ( *iter == '-' ) { // parse a long name
                std::string name( ++iter, arg.end());

                if ( flag_mapping.find(name) != flag_mapping.end() )
                    parsed.insert( make_pair(flag_mapping[name]->name,""));

                else if ( option_mapping.find(name)  != option_mapping.end()) {

                    if ( ++i == argc )
                        throw ParsingException("invalid use of option");

                    if ( ! option_mapping[name]->validate(std::string(argv[i])) )
                        throw ParsingException("invalid type");

                    parsed.insert( std::make_pair( option_mapping[name]->name, argv[i]) );
                }
                else
                    throw ParsingException(name + " is not a valid flag or option");

                continue;
            }

            // parse short name
            std::string character( iter, iter+1);

            if ( flag_mapping.find(character) != flag_mapping.end() )
            {
                parsed.insert( make_pair(flag_mapping[character]->name,""));
                iter++;

                for( ; iter != arg.end(); ++iter)
                {
                    std::string character( iter, iter+1);
                    if ( flag_mapping.find(character) != flag_mapping.end() )
                        parsed.insert( make_pair(flag_mapping[character]->name,""));
                    else if ( option_mapping.find(character)  != option_mapping.end()) 
                        throw ParsingException(arg + " is an invalid mix of flag and option");
                    else
                        throw ParsingException(character + " is not a valid flag or option");
                }
            }

            else if ( option_mapping.find(character)  != option_mapping.end()) {

                if ( iter+1 != arg.end() )
                    throw ParsingException("invalid use of option");

                if ( ++i == argc )
                    throw ParsingException("invalid use of option");

                if ( ! option_mapping[character]->validate(std::string(argv[i])) )
                    throw ParsingException("invalid type");

                parsed.insert( std::make_pair( option_mapping[character]->name, argv[i]) );
            }
            else
                throw ParsingException(character + " is not a valid flag or option");

            continue;
        } 

        if ( position > positionals.size() )
            throw ParsingException("too many positional arguments");

        if ( ! positionals[position]->validate(std::string(argv[i])) )
            throw ParsingException("invalid type");

        parsed.insert( std::make_pair(positionals[position++]->name, arg));
    }
}

bool ArgumentParser::has(const std::string& argument) const {
    return parsed.find(argument) != parsed.end();
}

template<typename T>
T ArgumentParser::get(const std::string& argument) const {
    std::istringstream iss( parsed.at(argument) );
    T ret;
    iss >> ret;
    return ret;
}

std::string ArgumentParser::get_message() const {

    std::string message("Usage: "+program_name);
    if  ( !options.empty() || !flags.empty() ) message += " {OPTIONS}";
    for (auto i : positionals)                 message += " [" + i->name + "]";

    message += "\n" + wrap_string(short_text,80);
    message += "\nOptions:\n";

    for (auto i : positionals) {
        std::string opt("    ");
        opt += i->name + " ";

        int gap;
        if ( opt.size() >= 20 ) {
            message+= opt + "\n";
            gap = 20;
        }
        else {
            message+= opt;
            gap = 20 - opt.size();
        }
        message += std::string(gap,' ');
        message += i->description;
        message += "\n";
    }

    for (auto i : options) {
        std::string opt("    ");
        for (auto o : i->opts) {
            if (o.size() == 1) opt += "-"  + o + " " + i->name + " ";
            else               opt += "--" + o + " " + i->name + " ";
        }

        int gap;
        if ( opt.size() >= 20 ) {
            message+= opt + "\n";
            gap = 20;
        }
        else {
            message+= opt;
            gap = 20 - opt.size();
        }
        message += std::string(gap,' ');
        message += i->description;
        message += "\n";
    }

    for (auto i : flags) {
        std::string opt("    ");
        for (auto o : i->flags) {
            if (o.size() == 1) opt += "-"  + o + " ";
            else               opt += "--" + o + " ";
        }

        int gap;
        if ( opt.size() >= 20 ) {
            message+= opt + "\n";
            gap = 20;
        }
        else {
            message+= opt;
            gap = 20 - opt.size();
        }
        message += std::string(gap,' ');
        message += i->description;
        message += "\n";
    }

    message+= "\n";
    message += wrap_string(long_text,80);
    return message;
}

std::ostream & operator<<(std::ostream &os, ArgumentParser const &a) {
    return os << a.get_message();
}

