#ifndef PEGPARSER_H_INCLUDED
#define PEGPARSER_H_INCLUDED

#include "peg.h"

#include <variant>

namespace peg
{

    // Generic parser with variant type value stack. 
    template <typename ...T>
    class Parser
    {
        using variant_type = std::variant<std::monostate, T...>;
        using stack_type = value_stack<variant_type>;

        Rule &start;
        matcher m;
        stack_type values;

    public:

        // Construct with starting rule and input stream (default std::cin)
        Parser(Rule &r, std::istream &in = std::cin) : start(r), m(in), values(m) { }

        // Parsing methods
        bool parse() { return start.parse(m); }
        void accept() { m.accept(); }
        void clear() { m.clear(); }
        std::string text() const { return m.text(); }

        // Reference to the value stored in a value stack slot. Throws std::bad_variant_access
        // if the slot does not currently hold a value of the required type.
        template <typename U> U &val(size_t idx) { return std::get<U>(values[idx]); }

        // Reference to a value stack slot
        variant_type &val(size_t idx) { return values[idx]; }

#ifdef PEG_DEBUG
        void check() const { start.check(); }
#endif
    };

    // Specialization for single type value stack. 
    template <typename T>
    class Parser<T>
    {
        using stack_type = value_stack<T>;

        Rule &start;
        matcher m;
        stack_type values;

    public:

        // Construct with starting rule and input stream (default std::cin)
        Parser(Rule &r, std::istream &in = std::cin) : start(r), m(in), values(m) { }

        // Parsing methods
        bool parse() { return start.parse(m); }
        void accept() { m.accept(); }
        void clear() { m.clear(); }
        std::string text() const { return m.text(); }

        // Reference to a value stack slot with explicit type qualification.
        // Provided for compatibility with code written for the generic case.
        template <typename U> U &val(size_t idx) { return values[idx]; }

        // Reference to a value stack slot
        T &val(size_t idx) { return values[idx]; }

#ifdef PEG_DEBUG
        void check() const { start.check(); }
#endif
    };

    // Specialization for no value stack
    template <>
    class Parser<>
    {
        Rule &start;
        matcher m;

    public:

        // Construct with starting rule and input stream (default std::cin)
        Parser(Rule &r, std::istream &in = std::cin) : start(r), m(in) { }

        // Parsing methods
        bool parse() { return start.parse(m); }
        void accept() { m.accept(); }
        void clear() { m.clear(); }
        std::string text() const { return m.text(); }

#ifdef PEG_DEBUG
        void check() const { start.check(); }
#endif
    };

}

#endif
