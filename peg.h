#ifndef PEG_H_INCLUDED
#define PEG_H_INCLUDED

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <bitset>
#include <utility>
#include <functional>
#include <exception>
#include <algorithm>
#include <codecvt>
#include <locale>
#include <variant>
#include <memory>
    

namespace peg
{
    class Expr;
    class Rule;

    namespace details
    {
        // An auto-resizing vector
        template <typename T> 
        class vect : public std::vector<T>
        {
            using vector = std::vector<T>;

        public:

            using vector::vector;

            T &operator[](std::size_t idx)
            {
                if ( idx >= vector::size() )
                    vector::resize(idx + 1);
                return vector::operator[](idx);
            }
        };

       // Lexical matcher, scheduled actions handler, text capture and value stack helper
        class matcher
        {
            // Constants
            static const unsigned BUFLEN = 1024;
            static const unsigned ACTSIZE = 32;
            static const unsigned ERRORLEN = 60;
            
            // Friends
            friend class peg::Expr;
            friend class peg::Rule;
            template <typename T> friend class value_stack;
            friend class parser;

            // Types
            class char_class  
            { 

                struct char_range 
                { 
                    char32_t low; 
                    char32_t high; 
        
                    char_range(char32_t lo, char32_t hi) : low(lo), high(hi) { }
                    bool operator<(const char_range &r) const { return high < r.low; }
                    void merge(const char_range &cr) 
                    {
                        if ( cr.low < low )
                            low = cr.low;
                        if ( cr.high > high )
                            high = cr.high;
                    }
                };

                static const unsigned NBITS = 256;
                std::bitset<NBITS> bs;              // optimization for the first NBITS characters  
                std::set<char_range> cs;            // for higher characters
                bool inverted = false;

                void add_range(char32_t lo, char32_t hi)
                {
                    // Use the bitset for low characters
                    while ( lo < NBITS && lo <= hi )
                        bs.set(lo++);

                    if ( lo > hi )
                        return;

                    // Use the range set for higher characters
                    char_range cr(lo, hi);

                    // Merge and remove overlapping ranges
                    for ( auto iter = cs.find(cr) ; iter != cs.end() ; iter = cs.find(cr) )
                    {
                        cr.merge(*iter);
                        cs.erase(iter);
                    }
     
                    // Insert aggregate range
                    cs.insert(cr);
                }

            public:

                char_class(const std::string &s) 
                {
                    // Convert s to a 32-bit string
                    struct cvt : std::codecvt<char32_t, char, std::mbstate_t> { };
                    std::wstring_convert<cvt, char32_t> converter;
                    std::u32string us = converter.from_bytes(s);

                    const char32_t *p = us.c_str(), *q = p + us.length(); 
         
                    if ( p == q )
                        return;

                    if ( p[0] == '^' )                      // inverted class
                    {
                        inverted = true;
                        p++;
                    }

                    while ( p < q )
                        if ( p + 2 < q && p[1] == '-' )     // character range
                        {
                            add_range(p[0], p[2]);
                            p += 3;
                        }
                        else                                // single character
                        {
                            add_range(p[0], p[0]);
                            p++;
                        }
                }

                bool find(char32_t value) const
                { 
                    // Lookup low values in the bitset, higher values in the range set.
                    bool found = value < NBITS ? bs[value] : cs.count(char_range(value, value));
                    return inverted ? !found : found; 
                }
            };
     
            struct mark { unsigned pos, actpos, begin, end; };
     
            struct action
            {
                std::function<void()> func;
                unsigned begin, end, base;
            };

            // Properties
            std::istream &in;
            std::string ibuf;
            char *buf = new char[BUFLEN];
            unsigned pos = 0;

            unsigned cap_begin = 0;
            unsigned cap_end = 0;

            vect<action> actions;
            unsigned actpos = 0;

            std::set<unsigned> lines;
            unsigned prev_lines = 0;

            unsigned level = 0;
            unsigned base = 0;

            unsigned error_pos = 0;
            std::string error_info;
            unsigned in_lah = 0;

           // Construct from an std::istream, default is std::cin.
            matcher(std::istream &is = std::cin) : in(is) { actions.reserve(ACTSIZE); }
            ~matcher() { delete[] buf; }
            matcher(const matcher &) = delete;                  // not copyable
            matcher &operator=(const matcher &) = delete;       // not assignable

            // Read a raw char from input.
            bool getc(char &c) 
            {
                if ( pos == ibuf.length() )     // try to get more input
                {
                    in.read(buf, BUFLEN);
                    int n = in.gcount();
                    if ( n <= 0 )
                        return false;
                    ibuf.append(buf, n);
                }

                if ( (c = ibuf[pos++]) == '\n' )
                    lines.insert(pos);
                return true;
            }

            // Read a 32-bit char from input.
            // Assumes (and does not check) correct utf8 encoding.
            bool getc32(char32_t &u)
            {
                char c;
                int n;

                if ( !getc(c) )
                    return false;

                u = c & 0xFF;

                if ( (c & 0xC0) != 0xC0 )   // not an utf8 sequence
                    return true;

                // Decode first byte of sequence
                switch ( (c >> 3) & 0x7 )
                {
                    case 0x0:               // 2-byte sequence
                    case 0x1:
                    case 0x2:
                    case 0x3:
                        u &= 0x1F;
                        n = 1;
                        break;

                    case 0x4:               // 3-byte sequence
                    case 0x5:
                        u &= 0x0F;
                        n = 2;
                        break;

                    case 0x6:               // 4-byte sequence
                        u &= 0x07;
                        n = 3;
                        break;

                    default:
                        return true;
                }

                while ( n-- )               // continuation bytes
                {
                    if ( !getc(c) )
                        return false;
                    u <<= 6 ;
                    u |= (c & 0x3F);
                }

                return true; 
            }

            // Matching primitives

            bool match_any() { char32_t c; return getc32(c); }

            bool match_string(const std::string &s)
            {
                char c;
                unsigned len = s.length();
                unsigned mpos = pos;

                for ( unsigned i = 0 ; i < len ; i++ )
                    if ( !getc(c) || c != s[i] )
                    {
                        pos = mpos;
                        return false;
                    }

                return true;
            }

            bool match_char(char32_t ch)
            {
                char32_t u;
                unsigned mpos = pos;

                if ( !getc32(u) || u != ch )
                {
                    pos = mpos;
                    return false;
                }

                return true;
            }

            bool match_class(const char_class &ccl)
            {
                char32_t u;
                unsigned mpos = pos;
     
                if ( !getc32(u) || !ccl.find(u) )
                {
                    pos = mpos;
                    return false;
                }

                return true;
            }

            // Schedule an action
            void schedule(std::function<void()> f)
            {
                if ( in_lah )
                    return;

                action &act = actions[actpos++];
                act.func = f;
                act.begin = cap_begin;
                act.end = cap_end;
                act.base = base;
            }

            // Set a mark and backtrack to it
            void set_mark(mark &mk) const { mk.pos = pos; mk.actpos = actpos; mk.begin = cap_begin; mk.end = cap_end; }
            void go_mark(const mark &mk) { pos = mk.pos; actpos = mk.actpos; cap_begin = mk.begin; cap_end = mk.end; }

            // Handle indices for automatic value stacks
            unsigned get_level() const { return level; }
            void set_level(unsigned n) { level = n; }

            unsigned get_base() const { return base; }
            void set_base(unsigned n) { base = n; }

            // Capture text
            unsigned begin_capture() const { return pos; }
            void end_capture(unsigned b) { cap_begin = b; cap_end = pos; }

            // Increase/decrease lookahead level
            void begin_lah() { in_lah++; }
            void end_lah() { in_lah--; }

            // Execute scheduled actions and consume matched input
            void accept() 
            { 
                for ( unsigned i = 0 ; i < actpos ; i++ )
                {
                    action &act = actions[i];
                    cap_begin = act.begin;
                    cap_end = act.end;
                    base = act.base;
                    act.func();
                }
     
                actpos = 0;

                ibuf.erase(0, pos); 
                pos = 0; 

                cap_begin = cap_end = 0;
                base = level = 0;

                prev_lines += lines.size();
                lines.clear();
                error_pos = 0;
                error_info = "";
                in_lah = 0;
            } 

            // Discard actions and input
            void clear() 
            { 
                actpos = 0;

                ibuf = "";
                pos = 0;

                cap_begin = cap_end = 0;
                base = level = 0;

                prev_lines = 0;
                lines.clear();
                error_pos = 0;
                error_info = "";
                in_lah = 0;
            }

            // Get last captured text
            std::string text() const { return ibuf.substr(cap_begin, cap_end - cap_begin); }

            // Set error info
            void set_error(const char *error) 
            { 
                if ( in_lah || pos < error_pos )
                    return;

                if ( pos  > error_pos )
                {
                    error_pos = pos;
                    error_info = error;
                }
                else  // pos == error_pos
                    error_info += error;
                error_info += ' ';
            }

            // Get error info
            std::string get_error() const
            { 
                unsigned line = prev_lines + 1;
                unsigned nlines = lines.size();
                if ( nlines )
                    for ( auto iter = lines.rbegin() ; nlines ; iter++, nlines-- )
                        if ( *iter <= error_pos )
                        {
                            line += nlines;
                            break;
                        }

                char buf[200];
                std::sprintf(buf,"Line %u\nExpecting ", line);
                return buf + error_info + "\nFound " + ibuf.substr(error_pos, ERRORLEN) + '\n';
            }

         };

        // Value stack 
        template <typename T>
        class value_stack
        {
            static const unsigned VALSIZE = 128;

            const matcher &mt;
            vect<T> values;

        public:

            value_stack(const matcher &m, std::size_t capacity = VALSIZE) : mt(m) { values.reserve(capacity); }
            T &operator[](std::size_t idx) { return values[mt.get_base() + idx]; }
        };

    } // namespace details

    // This class wraps a polimorphic expression pointer, 
    class Expr
    {
        // Friends
        friend Expr Lit(char32_t c);
        friend Expr Lit(const std::string &s);
        friend Expr Ccl(const std::string &s);
        friend Expr Any();
        friend Expr Do(std::function<void()> f);
        friend Expr Pred(std::function<void(bool &)> f);
        friend class Rule;

        // Syntax tree structures
        struct Expression 
        { 
            virtual unsigned size() const { return 1; }             // by default expressions use one value stack slot
            virtual bool parse(details::matcher &m) const = 0;
#ifdef PEG_DEBUG
            virtual void visit(unsigned &cons) const = 0;
#endif
            virtual ~Expression() = default;
        };

        using ExprPtr = std::shared_ptr<const Expression>;

        struct StrExpr : Expression         // string
        {
            std::string str;

            StrExpr(const std::string &s) : str(s) { }
            bool parse(details::matcher &m) const { return m.match_string(str); }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { cons += str.length(); }
#endif
        };

        struct ChrExpr : Expression         // single char
        {
            char32_t ch;

            ChrExpr(char32_t c) : ch(c) { }
            bool parse(details::matcher &m) const { return m.match_char(ch); }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { cons++; }
#endif
        };

        struct CclExpr : Expression         // char class
        {
            details::matcher::char_class ccl;

            CclExpr(const std::string &s) : ccl(s) { }
            bool parse(details::matcher &m) const { return m.match_class(ccl); }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { cons++; }
#endif
        };

        struct AnyExpr : Expression         // any character
        {
            bool parse(details::matcher &m) const { return m.match_any(); }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { cons++; }
#endif
        };

        struct LahExpr : Expression         // lookahead predicate
        {
            ExprPtr exp;
            unsigned siz;
            bool invert;

            LahExpr(ExprPtr e, bool inv) : exp(e), siz(e->size()), invert(inv) { };
            unsigned size() const { return siz; }
            bool parse(details::matcher &m) const 
            { 
                bool r;
                m.begin_lah();
                details::matcher::mark mk;
                m.set_mark(mk);
                if ( exp->parse(m) )
                {
                    m.go_mark(mk);
                    r = !invert;
                }
                else
                    r = invert;
                m.end_lah();
                return r;
            }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const 
            { 
                unsigned in = cons; 
                exp->visit(cons); 
                cons = in; 
            }
#endif
        };

        struct DoExpr : Expression          // action
        {
            std::function<void()> func;

            DoExpr(std::function<void()> f) : func(f) { }
            bool parse(details::matcher &m) const { m.schedule(func); return true; }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { }
#endif
        };

        struct PredExpr : Expression        // semantic predicate
        {
            std::function<void(bool &)> func;

            PredExpr(std::function<void(bool &)> f) : func(f) { }
            bool parse(details::matcher &m) const 
            {
                bool r = true; 
                func(r); 
                return r;
            }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { }
#endif
        };

        struct SeqExpr : Expression         // sequence
        {
            ExprPtr exp1, exp2;
            unsigned siz1, siz;

            SeqExpr(ExprPtr e1, ExprPtr e2) : exp1(e1), exp2(e2), siz1(e1->size()), siz(siz1 + e2->size()) { }
            unsigned size() const { return siz; }
            bool parse(details::matcher &m) const 
            { 
                details::matcher::mark mk;
                m.set_mark(mk);
                unsigned l = m.get_level();
                if ( !exp1->parse(m) )
                    return false;
                m.set_level(l + siz1);
                if ( !exp2->parse(m) )
                {
                    m.go_mark(mk);
                    m.set_level(l);
                    return false;
                }
                m.set_level(l);
                return true;
            }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { exp1->visit(cons); exp2->visit(cons); }
#endif
        };

        struct AttExpr : Expression         // attachment 
        {
            ExprPtr exp1, exp2;
            unsigned siz;

            AttExpr(ExprPtr e1, ExprPtr e2) : exp1(e1), exp2(e2), siz(e1->size()) { }
            unsigned size() const { return siz; }
            bool parse(details::matcher &m) const 
            { 
                details::matcher::mark mk;
                m.set_mark(mk);
                unsigned l = m.get_level();
                if ( !exp1->parse(m) )
                    return false;
                m.set_level(l + siz);
                if ( !exp2->parse(m) )
                {
                    m.go_mark(mk);
                    m.set_level(l);
                    return false;
                }
                m.set_level(l);
                return true;
            }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { exp1->visit(cons); exp2->visit(cons); }
#endif
        };

        struct AltExpr : Expression         // prioritized choice
        {
            ExprPtr exp1, exp2;
            unsigned siz;

            AltExpr(ExprPtr e1, ExprPtr e2) : exp1(e1), exp2(e2), siz(std::max(e1->size(), e2->size())) { }
            unsigned size() const { return siz; }
            bool parse(details::matcher &m) const { return exp1->parse(m) || exp2->parse(m); }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const 
            {
                unsigned in = cons; 
                exp1->visit(cons);
                unsigned out = cons;
                cons = in; 
                exp2->visit(cons); 
                if ( out < cons )
                    cons = out;
            }
#endif
        };

        struct RepExpr : Expression         // repetition
        {
            ExprPtr exp;
            unsigned nmin, nmax;
            unsigned siz;

            RepExpr(ExprPtr e, unsigned nmin, unsigned nmax) : exp(e), nmin(nmin), nmax(nmax), siz(e->size()) { };
            unsigned size() const { return siz; }
            bool parse(details::matcher &m) const
            {
                unsigned n;

                if ( nmin == 1 )
                {
                    if ( !exp->parse(m) )
                        return false;
                    n = 1;
                }
                else if ( nmin > 1 )
                {
                    details::matcher::mark mk;
                    m.set_mark(mk);
                    for ( n = 0 ; n < nmin ; n++ ) 
                        if ( !exp->parse(m) )
                        {
                            if ( n )
                                m.go_mark(mk);
                            return false;
                        }
                }
                else
                    n = 0;

                if ( nmax )
                    while ( n < nmax && exp->parse(m) )
                        n++;
                else 
                     while ( exp->parse(m) )
                        ;
                    
               return true;
            }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const 
            {
                if ( nmin ) 
                    exp->visit(cons); 
                else
                {
                    unsigned in = cons;
                    exp->visit(cons); 
                    cons = in;
                }
            }
#endif
        };

        struct CapExpr : Expression         // text capture
        {
            ExprPtr exp;
            unsigned siz;

            CapExpr(ExprPtr e) : exp(e), siz(e->size()) { }
            unsigned size() const { return siz; }
            bool parse(details::matcher &m) const
            {
                unsigned b = m.begin_capture();
                bool r = exp->parse(m);
                if ( r )
                    m.end_capture(b);
                return r;
            }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { exp->visit(cons); }
#endif
        };

        // auxiliary class for operator[]
        struct range
        {
            unsigned nmin, nmax;
            range(unsigned n) : nmin(n), nmax(n) { }
            range(unsigned nmin, unsigned nmax) : nmin(nmin), nmax(nmax) { }
        };

        // The wrapped pointer.
        ExprPtr exp;

        // Constructors, etc. 
        Expr() = delete;
        Expr(const Expr &r) = default;
        Expr &operator=(const Expr &r) = default;

        Expr(const Expression *e) : exp(e) { }                                                      // from raw expression pointer
        operator ExprPtr() { return exp; }                                                          // to expression pointer

        Expr(const std::string &s) : exp(new StrExpr(s)) { }                                        // from string
        Expr(char32_t c) : exp(new ChrExpr(c)) { }                                                  // from single char
        Expr(std::function<void()> f) : exp(new DoExpr(f)) { }                                      // from action
        Expr(std::function<void(bool &)> f) : exp(new PredExpr(f)) { }                              // from semantic predicate

    public:

        // Prefix operators
        Expr operator*()  const { return new RepExpr(exp, 0, 0); }                                  // zero or more times
        Expr operator+()  const { return new RepExpr(exp, 1, 0); }                                  // one or more times
        Expr operator~()  const { return new RepExpr(exp, 0, 1); }                                  // optional
        Expr operator&()  const { return new LahExpr(exp, false); }                                 // and-predicate (1)
        Expr operator!()  const { return new LahExpr(exp, true); }                                  // not-predicate
        Expr operator--() const { return new CapExpr(exp); }                                        // text capture

        // (1) Overloads unary &

        // Postfix operators
        Expr operator--(int) const { return new CapExpr(exp); }                                     // text capture
        template <typename T> Expr operator()(const T &t) const                                     // attachment
        { 
            return new AttExpr(exp, Expr(t)); 
        }
        Expr operator[](range r) { return new RepExpr(exp, r.nmin, r.nmax); }                       // repetition

        // Binary operators
        template <typename T, typename U> friend Expr operator>>(const T &t, const U &u)            // sequence
        { 
            return new SeqExpr(Expr(t), Expr(u)); 
        } 
        template <typename T, typename U> friend Expr operator|(const T &t, const U &u)             // ordered choice
        { 
            return new AltExpr(Expr(t), Expr(u)); 
        } 
    };

    // Lexical primitives
    inline Expr Lit(const std::string &s) { return Expr(s); }                                       // string
    inline Expr Lit(char32_t c) { return Expr(c); }                                                 // single char
    inline Expr Ccl(const std::string &s) { return new Expr::CclExpr(s); }                          // char class
    inline Expr Any() { return new Expr::AnyExpr; }                                                 // any char
    inline Expr Do(std::function<void()> f) { return Expr(f); }                                     // action
    inline Expr Pred(std::function<void(bool &)> f) { return Expr(f); }                             // semantic predicate

    // Literals
    inline namespace literals
    {
        inline Expr operator""_lit(char c) { return Lit(c); }                                           // single char
        inline Expr operator""_lit(char32_t c) { return Lit(c); }                                       // single char
        inline Expr operator""_lit(const char *s, std::size_t len) { return Lit(std::string(s, len)); } // string
        inline Expr operator""_ccl(const char *s, std::size_t len) { return Ccl(std::string(s, len)); } // char class
    }

    // Grammar rules 
    class Rule : public Expr
    {
        // The root of this rule's expression tree
        ExprPtr root;

        const char *label;  // for error reporting

        // A rule expression is a structure that holds a reference to the rule. 
        // This indirection allows rules to refer to other rules before they are defined.
        struct RuleExpr : Expression
        {
            Rule &rule;

            RuleExpr(Rule &r) : rule(r) { }
            bool parse(details::matcher &m) const { return rule.parse(m); }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { rule.visit(cons); }
#endif
        };

#ifdef PEG_DEBUG
        // Variables for rule visitor
        bool visiting = false, visited = false; 
        unsigned my_cons;
        const char *name = nullptr;

        // Visit the paths from this rule
        void visit(unsigned &cons)  
        {
            static unsigned level;

            if ( name )
            { 
                for ( unsigned i = 0 ; i < level ; i++ )
                    std::cerr << "| "; 
                std::cerr << name << (visited ? " (v)" : visiting ? " (r)" : "") << std::endl;
                level++;
            }

            if ( visited )
                cons += my_cons;
            else if ( !visiting )
            {
                if ( !root )
                    throw bad_rule("Uninitialized rule");                    
                my_cons = cons;
                visiting = true;
                root->visit(cons);
                visited = true;
                my_cons = cons - my_cons;   // what this rule consumes
            }
            else                            // recursing...
                if ( cons == my_cons )      // without consuming input
                    throw bad_rule("Left-recursive rule");

            if ( name ) 
                level--;
        }
#endif

    public:

        // Default constructor. No copying.
        Rule(const char *lbl = nullptr) : Expr(new RuleExpr(*this)), label(lbl) { }
        Rule(const Rule &) = delete;

        // Assign from an expression.
        // Note r = r is not trivial, it makes r left-recursive.
        template <typename T> Rule &operator=(const T &t) { root = Expr(t); return *this; }

        // Exceptions.
        class bad_rule : public std::exception
        {
            const char *str;

        public:

            bad_rule(const char *s) : str(s) { }
            const char *what() const noexcept { return str; }
        };

        // Parse the root adjusting the base of value stack indices.
        bool parse(details::matcher &m) const 
        { 
            if ( !root )
                throw bad_rule("Uninitialized rule");
            unsigned base = m.get_base();
            m.set_base(m.get_level());
            bool r = root->parse(m);
            if ( label && !r )
                m.set_error(label);
            m.set_base(base);
            return r;
        }

#ifdef PEG_DEBUG
        // Enable debugging.
        void set_name(const char *debug_name) { name = debug_name; };

        // Check the grammar starting here for uninitialized rules and 
        // left recursion. This can be done only once.
        void check()
        {
            unsigned cons = 0;
            visit(cons);
            if ( name )
                std::cerr << name << ": ";
            std::cerr << "check OK\n";
        }
#endif
    };

    namespace details
    {
        class parser
        {
            Rule &__start;

        public:

            details::matcher __m;

            // Construct with starting rule and input stream
            parser(Rule &r, std::istream &in = std::cin) : __start(r), __m(in) { }

            // Parsing methods
            bool parse() { return __start.parse(__m); }
            void accept() { __m.accept(); }
            void clear() { __m.clear(); }
            std::string text() const { return __m.text(); }
            std::string get_error() const { return __m.get_error(); } 

    #ifdef PEG_DEBUG
            // Grammar check
            void check() const { __start.check(); }
    #endif
        };
    }

    // Parser 
    template <typename T = void>
    class Parser : public details::parser
    {
        details::value_stack<T> __values;

    public:

        // Construct with starting rule and input stream
        Parser(Rule &r, std::istream &in = std::cin) : details::parser(r, in), __values(__m) { }
        Parser(Rule &r, std::size_t capacity, std::istream &in = std::cin) : details::parser(r, in), __values(__m, capacity) { }

        // Reference to a value stack slot
        T &val(std::size_t idx) { return __values[idx]; }
        const T &val(std::size_t idx) const { return __values[idx]; }

        // Reference to a value contained in a variant type value stack slot
        template <typename U> U &val(std::size_t idx) { return std::get<U>(__values[idx]); }
        template <typename U> const U &val(std::size_t idx) const { return std::get<U>(__values[idx]); }
    };

    template < >
    class Parser <void> : public details::parser
    {
    public:

        using details::parser::parser;
    };

} // namespace peg

#ifdef PEG_DEBUG
// Set a rule's name for debugging
#define peg_debug(rule)     rule.set_name(#rule)
#endif

// Semantic actions and predicates
#define do_(...)            (peg::Do([&]{ __VA_ARGS__ }))  
#define pa_(...)            (peg::Pred([&](bool &){ __VA_ARGS__ }))  
#define pr_(...)            (peg::Pred([&](bool &__r){ __r = [&]()->bool{ __VA_ARGS__ }(); }))  
#define if_(...)            (peg::Pred([&](bool &__r){ __r = (__VA_ARGS__); }))

#endif
