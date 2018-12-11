#ifndef PEGPARSER_H_INCLUDED
#define PEGPARSER_H_INCLUDED

#include "peg.h"

#include <variant>

namespace peg
{

     // Basic parser without value stack
    class Parser
    {
        Rule &__start;

    protected:

        matcher __m;

    public:

        // Construct with starting rule and input stream
        Parser(Rule &r, std::istream &in = std::cin) : __start(r), __m(in) { }

        // Parsing methods
        bool parse() { return __start.parse(__m); }
        void accept() { __m.accept(); }
        void clear() { __m.clear(); }
        std::string text() const { return __m.text(); }

#ifdef PEG_DEBUG
        // Grammar check
        void check() const { __start.check(); }
#endif
    };

#ifdef PEGPARSER_USE_MAP
    template <typename T> using stack_type = value_stack<T>;
#else
    template <typename T> using stack_type = value_map<T>;
#endif

    // Parser with variant type value stack. 
    template <typename ...T>
    class VParser : public Parser
    {
        using element_type = std::variant<std::monostate, T...>;
        value_stack<element_type> __values;

    public:

        // Construct with starting rule and input stream
        VParser(Rule &r, std::istream &in = std::cin) : Parser(r, in), __values(__m) { }

        // Reference to a value stack slot
        element_type &val(std::size_t idx) { return __values[idx]; }

        // Reference to the value stored in a value stack slot. Throws std::bad_variant_access
        // if the slot does not currently hold a value of the required type.
        template <typename U> U &val(std::size_t idx) { return std::get<U>(__values[idx]); }
    };

    // Parser with single type value stack. 
    template <typename T>
    class TParser : public Parser
    {
        value_stack<T> __values;

    public:

        // Construct with starting rule and input stream
        TParser(Rule &r, std::istream &in = std::cin) : Parser(r, in), __values(__m) { }

        // Reference to a value stack slot
        T &val(std::size_t idx) { return __values[idx]; }

        // Reference to a value stack slot with explicit type qualification.
        // Provided for compatibility with the variant case.
        template <typename U> U &val(std::size_t idx) { return __values[idx]; }
    };

}

#endif
