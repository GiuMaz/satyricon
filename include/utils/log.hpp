#ifndef SATYRICON_LOG_HPP
#define SATYRICON_LOG_HPP

#include <iostream>

namespace Utils {

enum log_level {LOG_NONE=0,LOG_NORMAL=1,LOG_VERBOSE=2};

class Log {
public:

    Log() : normal(0), verbose(0), level(LOG_NONE) {}
    Log(log_level lv) : normal(0), verbose(0), level(lv) { set_level(lv); }
    Log(const Log& other) :
        normal(other.normal.rdbuf()),
        verbose(other.verbose.rdbuf()),
        level(other.level) {}

    void set_level( log_level lv) {
        level = lv;
        if ( level >= LOG_NORMAL )
            normal.rdbuf(std::cout.rdbuf());
        if ( level >= LOG_VERBOSE )
            verbose.rdbuf(std::cout.rdbuf());
    }

    Log& operator=(const Log& other) {
        set_level(other.level);
        return *this;
    }

    std::ostream normal;
    std::ostream verbose;

private:
    log_level level;
};


} // end of namespace Utils

#endif
