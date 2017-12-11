#ifndef ASSERT_MESSAGE_HH
#define ASSERT_MESSAGE_HH

#include <iostream>
#include <string>

namespace Utils {

#ifndef NDEBUG
#   define assert_message(Expr, Msg) \
    Utils::__M_Assert(#Expr, Expr, __FILE__, __LINE__, Msg)

/**
 * assertion with error message, only in debug mode
 */
inline void __M_Assert(const char* expr_str, bool expr,
        const char* file, int line, std::string msg) {
    if (!expr) {
        std::cerr << "Failed:   " << msg << std::endl
                  << "Expected: " << expr_str << std::endl
                  << "Source:   " << file << ", line " << line << std::endl;
        abort();
    }
}

#else
#   define assert_message(Expr, Msg) ;
#endif

} // end namespace Utils

#endif
