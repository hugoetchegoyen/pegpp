#ifndef PEGPARSER_H_INCLUDED
#define PEGPARSER_H_INCLUDED

#include "peg.h"

#include <variant>

namespace peg
{

    namespace details
    {

        // Basic parser with starting rule and matcher
        class __parser
        {
        protected:

            Rule &__start;
            matcher __m;

            // Construct with starting rule and input stream
            __parser(Rule &r, std::istream &in) : __start(r), __m(in) { }

        public:

            // Parsing methods
            bool parse() { return __start.parse(__m); }
            void accept() { __m.accept(); }
            void clear() { __m.clear(); }
            std::string text() const { return __m.text(); }

    #ifdef PEG_DEBUG
            // Grammar checking
            void check() const { __start.check(); }
    #endif
        };

    }

    // Generic parser with variant type value stack. 
    template <typename ...T>
    class Parser : public details::__parser
    {
        using variant_type = std::variant<std::monostate, T...>;
        using stack_type = value_stack<variant_type>;

        stack_type __values;

    public:

        // Construct with starting rule and input stream (default std::cin)
        Parser(Rule &r, std::istream &in = std::cin) : __parser(r, in), __values(__m) { }

        // Reference to the value stored in a value stack slot. Throws std::bad_variant_access
        // if the slot does not currently hold a value of the required type.
        template <typename U> U &val(size_t idx) { return std::get<U>(__values[idx]); }

        // Reference to a value stack slot
        variant_type &val(size_t idx) { return __values[idx]; }
    };

    // Specialization for single type value stack. 
    template <typename T>
    class Parser<T> : public details::__parser
    {
        using stack_type = value_stack<T>;

        stack_type __values;

    public:

        // Construct with starting rule and input stream (default std::cin)
        Parser(Rule &r, std::istream &in = std::cin) : __parser(r, in), __values(__m) { }

        // Reference to a value stack slot with explicit type qualification.
        // Provided for compatibility with code written for the generic case.
        template <typename U> U &val(size_t idx) { return __values[idx]; }

        // Reference to a value stack slot
        T &val(size_t idx) { return __values[idx]; }
    };

    // Specialization for no value stack
    template <>
    class Parser<> : public details::__parser
    {
    public:

        // Construct with starting rule and input stream (default std::cin)
        Parser(Rule &r, std::istream &in = std::cin) : __parser(r, in) { }
    };

}

#endif
