#ifndef PEG_H_INCLUDED
#define PEG_H_INCLUDED

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <bitset>
#include <utility>
#include <functional>
#include <exception>
#include <algorithm>

#ifdef PEG_USE_SHARED_PTR
#include <memory>
#endif

namespace peg
{
    // Auxiliary vector class
    template <typename T> 
    class vect
    {
        std::vector<T> v;

    public:

        struct bad_index : public std::exception
        {
            const char *what() const noexcept { return "Out of bounds index"; }
        };

        void reserve(unsigned cap) { v.reserve(cap); }
        unsigned size() const { return v.size(); }
        void resize(unsigned size) { v.resize(size); }
        void clear() { v.clear(); }

        T &operator[](int idx)
        {
            if ( idx < 0 )
                throw bad_index();
            if ( idx >= (int) v.size() )
                v.resize(idx + 1);
            return v[idx];
        }

        T &top(int idx = 0) { return operator[](v.size() - 1 + idx); }

        void push(const T& t) { v.push_back(t); }
        void push(T&& t) { v.push_back(std::move(t)); }
        void pop(unsigned count = 1) { v.resize(v.size() - count); }
    };

    // Lexical matcher, scheduled actions handler, text capture and value stack helper
    class matcher
    {
        // Constants
        static const unsigned BUFLEN = 1024;
        static const unsigned ACTSIZE = 32;
        
        // Friends
        friend class Expr;
        friend class Rule;
        template <typename T> friend class value_stack;
        template <typename T> friend class value_map;

        // Types
        typedef std::bitset<256> char_class;
        struct mark { unsigned pos, actpos, begin, end; };
        struct action
        {
            std::function<void()> func;
            unsigned begin, end, base, level;
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

        unsigned level = 0;
        unsigned base = 0;

        // Make a character class from a string
        static char_class make_class(const std::string &s) 
        {
            char_class ccl;
            const char *p = s.c_str(), *q = p + s.length(); 
            bool value = true;

            if ( *p == '^' )
            {
                ccl.set();
                value = false;
                p++;
            }
            while ( p < q )
            {
                unsigned c = *p++ & 0xFF;
                ccl.set(c, value);
                if ( p + 1 < q && *p == '-' )
                {
                    unsigned d = p[1] & 0xFF;
                    while ( c < d )
                        ccl.set(++c, value);
                    p += 2;
                }
            }
            return ccl;
        }
    
        // Get one char
        bool getc(int &c) 
        {
            if ( pos == ibuf.length() )                     // try to get more input
            {
                in.read(buf, BUFLEN);
                int n = in.gcount();
                if ( n <= 0 )
                    return false;
                ibuf.append(buf, n);
            }
            c = ibuf[pos++]; 
            return true;
        }

        // Matching primitives

        bool match_any() { int c; return getc(c); }

        bool match_string(const std::string &s)
        {
            int c;
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

        bool match_char(char ch)
        {
            int c;
            unsigned mpos = pos;
            if ( !getc(c) || c != ch )
            {
                pos = mpos;
                return false;
            }
            return true;
        }

        bool match_class(const char_class ccl)
        {
            int c;
            unsigned mpos = pos;
            if ( !getc(c) || !ccl[c & 0xFF] )
            {
                pos = mpos;
                return false;
            }
            return true;
        }

        // Schedule an action
        void schedule(std::function<void()> f)
        {
            action &act = actions[actpos++];
            act.func = f;
            act.begin = cap_begin;
            act.end = cap_end;
            act.base = base;
            act.level = level;
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

    public:

        // Construct from an std::istream, default is std::cin.
        matcher(std::istream &is = std::cin) : in(is) { actions.reserve(ACTSIZE); }
        ~matcher() { delete[] buf; }
        matcher(const matcher &) = delete;                  // not copyable
        matcher &operator=(const matcher &) = delete;       // not assignable

        // Execute scheduled actions and consume matched input
        void accept() 
        { 
            for ( unsigned i = 0 ; i < actpos ; i++ )
            {
                action &act = actions[i];
                cap_begin = act.begin;
                cap_end = act.end;
                base = act.base;
                level = act.level;
                act.func();
            }
            actpos = 0;

            ibuf.erase(0, pos); 
            pos = 0; 

            cap_begin = cap_end = 0;
            base = level = 0;
        } 

        // Discard actions and input
        void clear() 
        { 
            actpos = 0;

            ibuf = "";
            pos = 0;

            cap_begin = cap_end = 0;
            base = level = 0;
        }

        // Get last captured text
        std::string text() const { return ibuf.substr(cap_begin, cap_end - cap_begin); }
    };

    // Value stack classes

    template <typename T>
    class value_stack
    {
        vect<T> values;
        const matcher &mt;
        static const unsigned VALSIZE = 128;

    public:

        value_stack(const matcher &m, unsigned n = VALSIZE)  : mt(m) { values.reserve(n); }

        T &operator[](int idx) { return values[mt.get_base() + idx]; }
		void clear() { values.clear(); }
    };

    template <typename T>
    class value_map
    {
        std::map<int, T> values;
        const matcher &mt;

    public:

        struct bad_index : public std::exception
        {
            const char *what() const noexcept { return "Out of bounds index"; }
        };

        value_map(const matcher &m)  : mt(m) { }

        T &operator[](int idx) 
        {
            idx += mt.get_base();
            if ( idx < 0 )
                throw bad_index(); 
            return values[idx]; 
        }
		void clear() { values.clear(); }
    };

    // Forward declarations
    class Expr; 

    inline namespace literals
    {
        inline Expr operator""_lit(const char *s, std::size_t len);
        inline Expr operator""_lit(char c);
        inline Expr operator""_ccl(const char *s, std::size_t len);
    }

    // This class is a wrapper around a polimorphic expression pointer, 
    // We need it to define operators for building a syntax tree.
    class Expr
    {
        // Friends
        friend Expr Lit(const std::string &s);
        friend Expr Lit(char c);
        friend Expr Ccl(const std::string &s);
        friend Expr Any();
        friend Expr Do(std::function<void()> f);
        friend Expr Pred(std::function<void(bool &)> f);

        friend Expr literals::operator""_lit(const char *s, std::size_t len);
        friend Expr literals::operator""_lit(char c);
        friend Expr literals::operator""_ccl(const char *s, std::size_t len);

        friend class Rule;

        // Syntax tree structures
        struct Expression 
        { 
            virtual bool parse(matcher &m) const = 0;
            virtual unsigned size() const { return 1; }             // by default expressions use one value stack slot
#ifdef PEG_DEBUG
            virtual void visit(unsigned &cons) const = 0;
#endif
            virtual ~Expression() = default;
        };

#ifdef PEG_USE_SHARED_PTR
        using ExprPtr = std::shared_ptr<const Expression>;
#else
        using ExprPtr = const Expression *;
#endif

        struct StrExpr : Expression         // string
        {
            std::string str;

            StrExpr(const std::string &s) : str(s) { }
            bool parse(matcher &m) const { return m.match_string(str); }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { cons += str.length(); }
#endif
        };

        struct ChrExpr : Expression         // single char
        {
            char ch;

            ChrExpr(char c) : ch(c) { }
            bool parse(matcher &m) const { return m.match_char(ch); }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { cons++; }
#endif
        };

        struct CclExpr : Expression         // char class
        {
            matcher::char_class ccl;

            CclExpr(const std::string &s) : ccl(matcher::make_class(s)) { }
            bool parse(matcher &m) const { return m.match_class(ccl); }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { cons++; }
#endif
        };

        struct AnyExpr : Expression         // any character
        {
            bool parse(matcher &m) const { return m.match_any(); }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { cons++; }
#endif
        };

        struct AndExpr : Expression         // and-predicate
        {
            ExprPtr exp;
            unsigned siz;

            AndExpr(ExprPtr e) : exp(e), siz(e->size()) { };
            unsigned size() const { return siz; }
            bool parse(matcher &m) const 
            { 
                matcher::mark mk;
                m.set_mark(mk);
                if ( exp->parse(m) )
                {
                    m.go_mark(mk);
                    return true;
                } 
                return false;
            }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { exp->visit(cons); }
#endif
        };

        struct NotExpr : Expression         // not-predicate
        {
            ExprPtr exp;
            unsigned siz;

            NotExpr(ExprPtr e) : exp(e), siz(e->size()) { };
            unsigned size() const { return siz; }
            bool parse(matcher &m) const 
            { 
                matcher::mark mk;
                m.set_mark(mk);
                if ( exp->parse(m) )
                {
                    m.go_mark(mk);
                    return false;
                } 
                return true;
            }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { exp->visit(cons); }
#endif
        };

        struct DoExpr : Expression          // action
        {
            std::function<void()> func;

            DoExpr(std::function<void()> f) : func(f) { }
            bool parse(matcher &m) const { m.schedule(func); return true; }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { }
#endif
        };

        struct PredExpr : Expression        // semantic predicate
        {
            std::function<void(bool &)> func;

            PredExpr(std::function<void(bool &)> f) : func(f) { }
            bool parse(matcher &m) const 
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
            bool parse(matcher &m) const 
            { 
                matcher::mark mk;
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
            bool parse(matcher &m) const 
            { 
                matcher::mark mk;
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
            bool parse(matcher &m) const { return exp1->parse(m) || exp2->parse(m); }
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

        struct ZomExpr : Expression         // zero or more times
        {
            ExprPtr exp;
            unsigned siz;

            ZomExpr(ExprPtr e) : exp(e), siz(e->size()) { };
            unsigned size() const { return siz; }
            bool parse(matcher &m) const
            {
                while ( exp->parse(m) )
                    ;
                return true;
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

        struct OomExpr : Expression         // one or more times
        {
            ExprPtr exp;
            unsigned siz;

            OomExpr(ExprPtr e) : exp(e), siz(e->size()) { };
            unsigned size() const { return siz; }
            bool parse(matcher &m) const
            {
                if ( !exp->parse(m) )
                    return false;
                while ( exp->parse(m) )
                    ;
                return true;
            }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { exp->visit(cons); }
#endif
        };

        struct OptExpr : Expression         // optional
        {
            ExprPtr exp;
            unsigned siz;

            OptExpr(ExprPtr e) : exp(e), siz(e->size()) { };
            unsigned size() const { return siz; }
            bool parse(matcher &m) const { exp->parse(m); return true; }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const 
            { 
                unsigned in = cons;
                exp->visit(cons); 
                cons = in; 
            }
#endif
        };

        struct CapExpr : Expression         // text capture
        {
            ExprPtr exp;
            unsigned siz;

            CapExpr(ExprPtr e) : exp(e), siz(e->size()) { }
            unsigned size() const { return siz; }
            bool parse(matcher &m) const
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

        // The wrapped pointer.
        ExprPtr exp;

        // Constructors, etc. 
        Expr() = delete;
        Expr(const Expr &r) = default;
        Expr &operator=(const Expr &r) = default;

        Expr(ExprPtr e) : exp(e) { }                                                                // from expression pointer
        Expr(const std::string &s) : exp(ExprPtr(new StrExpr(s))) { }                               // from string
        Expr(char c) : exp(ExprPtr(new ChrExpr(c))) { }                                             // from single char
        Expr(std::function<void()> f) : exp(ExprPtr(new DoExpr(f))) { }                             // from action
        Expr(std::function<void(bool &)> f) : exp(ExprPtr(new PredExpr(f))) { }                     // from semantic predicate

    public:

        // Prefix operators
        Expr operator*() const { return ExprPtr(new ZomExpr(exp)); }                                // zero or more times
        Expr operator+() const { return ExprPtr(new OomExpr(exp)); }                                // one or more times
        Expr operator~() const { return ExprPtr(new OptExpr(exp)); }                                // optional
        Expr operator&() const { return ExprPtr(new AndExpr(exp)); }                                // and-predicate (1)
        Expr operator!() const { return ExprPtr(new NotExpr(exp)); }                                // not-predicate
        Expr operator--() const { return ExprPtr(new CapExpr(exp)); }                               // text capture

        // (1) Overloads unary &

        // Postfix operators
        Expr operator--(int) const { return ExprPtr(new CapExpr(exp)); }                            // text capture
        Expr operator()(const Expr &r) { return ExprPtr(new AttExpr(exp, r.exp)); }                 // attach expression
        Expr operator()(const std::string &s) { return operator()(Expr(s)); }                       // attach string
        Expr operator()(char c) { return operator()(Expr(c)); }                                     // attach single char
        Expr operator()(std::function<void()> f) { return operator()(Expr(f)); }                    // attach action
        Expr operator()(std::function<void(bool &)> f) { return operator()(Expr(f)); }              // attach semantic predicate

        // Binary operators 
        Expr operator>>(const Expr &r) const { return ExprPtr(new SeqExpr(exp, r.exp)); }           // sequence
        Expr operator|(const Expr &r) const { return ExprPtr(new AltExpr(exp, r.exp)); }            // ordered choice

        // Overloaded binary operators
        friend Expr operator>>(const Expr &r, const std::string &s) { return r >> Expr(s); }        // string overloads
        friend Expr operator>>(const std::string &s, const Expr &r) { return Expr(s) >> r; }
        friend Expr operator|(const Expr &r, const std::string &s) { return r | Expr(s); } 
        friend Expr operator|(const std::string &s, const Expr &r) { return Expr(s) | r; } 

        friend Expr operator>>(const Expr &r, char c) { return r >> Expr(c); }                      // single char overloads
        friend Expr operator>>(char c, const Expr &r) { return Expr(c) >> r; }
        friend Expr operator|(const Expr &r, char c) { return r | Expr(c); }
        friend Expr operator|(char c, const Expr &r) { return Expr(c) | r; }

        friend Expr operator>>(const Expr &r, std::function<void()> f) { return r >> Expr(f); }     // action overloads
        friend Expr operator>>(std::function<void()> f, const Expr &r) { return Expr(f) >> r; }
        friend Expr operator|(const Expr &r, std::function<void()> f) { return r | Expr(f); }
        friend Expr operator|(std::function<void()> f, const Expr &r) { return Expr(f) | r; }

        friend Expr operator>>(const Expr &r, std::function<void(bool &)> f) { return r >> Expr(f); }   // semantic predicate
        friend Expr operator>>(std::function<void(bool &)> f, const Expr &r) { return Expr(f) >> r; }   // overloads
        friend Expr operator|(const Expr &r, std::function<void(bool &)> f) { return r | Expr(f); }
        friend Expr operator|(std::function<void(bool &)> f, const Expr &r) { return Expr(f) | r; }

    };

    // Lexical primitives
    inline Expr Lit(const std::string &s) { return Expr(s); }                                       // string
    inline Expr Lit(char c) { return Expr(c); }                                                     // single char
    inline Expr Ccl(const std::string &s) { return Expr::ExprPtr(new Expr::CclExpr(s)); }           // char class
    inline Expr Any() { return Expr::ExprPtr(new Expr::AnyExpr); }                                  // any char
    inline Expr Do(std::function<void()> f) { return Expr(f); }                                     // action
    inline Expr Pred(std::function<void(bool &)> f) { return Expr(f); }                             // semantic predicate

    // Literals
    inline namespace literals
    {
        inline Expr operator""_lit(const char *s, std::size_t len) { return Lit(std::string(s, len)); }     // string
        inline Expr operator""_lit(char c) { return Lit(c); }                                               // single char
        inline Expr operator""_ccl(const char *s, std::size_t len) { return Ccl(std::string(s, len)); }     // char class
    }

    // Grammar rules 
    class Rule : public Expr
    {
        // The root of this rule's expression tree
        ExprPtr root = nullptr;

#ifdef PEG_DEBUG
        // Variables for rule visitor
        bool visiting = false, visited = false; 
        unsigned my_cons;
#endif

        // A rule expression is a structure that holds a reference to the rule. 
        // This indirection allows rules to refer to other rules before they are defined.
        struct RuleExpr : Expression
        {
            Rule &rule;

            RuleExpr(Rule &r) : rule(r) { }
            bool parse(matcher &m) const { return rule.parse(m); }
#ifdef PEG_DEBUG
            void visit(unsigned &cons) const { rule.visit(cons); }
#endif
        };

#ifdef PEG_DEBUG
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

        // Exceptions thrown
        class bad_rule : public std::exception
        {
            const char *str;

        public:

            bad_rule(const char *s) : str(s) { }
            const char *what() const noexcept { return str; }
        };

        // Adjust the base of value stack indices and parse the root.
        bool parse(matcher &m) const 
        { 
            if ( !root )
                throw bad_rule("Uninitialized rule");
            unsigned base = m.get_base();
            m.set_base(m.get_level());
            bool r = root->parse(m);
            m.set_base(base);
            return r;
        }

#ifdef PEG_DEBUG
        // Rule name
        const char *name = nullptr;

        // Check the grammar starting here for uninitialized rules and 
        // left recursion. Can be done only once.
        void check()
        {
            unsigned cons = 0;
            visit(cons);
            if ( name )
                std::cerr << name << ": check OK" << std::endl;
            else
                std::cerr << "check OK" << std::endl;
        }
#endif

        // Only default construction allowed. No coying.
        Rule() : Expr(ExprPtr(new RuleExpr(*this))) { }
        Rule(const Rule &r) = delete;

        // An expression assigns the root.
        Rule &operator=(const Expr &r) { root = r.exp; return *this; }

        // Copy assignment takes the source rule's expression.
        //      Rule r;
        //      r = r;
        // This is non-trivial, it makes r left-recursive.
        Rule &operator=(const Rule &r) { return *this = Expr(r); }

        // Overloaded assignments for:
        //      string
        //      const char *
        //      char
        //      action
        //      semantic predicate
        template <typename T> Rule &operator=(T t) { return *this = Expr(t); }
    };

}; 

#ifdef PEG_DEBUG
#define peg_debug(rule)     rule.name = #rule
#endif

#define _(code...)          (peg::Do([&]{ code }))  
#define __(code...)         (peg::Pred([&](bool &__r){ __r = ([&]()-> bool { code })(); }))  

#endif // PEG_H_INCLUDED
