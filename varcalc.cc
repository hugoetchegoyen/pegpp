// A floating point calculator supporting exponentiation and named variables.

#include <iostream>
#include <string>
#include <map>

#include <math.h>

// #define PEG_DEBUG            // Uncomment for checking the grammar

#include "peg.h"

using namespace std;
using namespace peg;

class calculator : public Parser<double, string>
{
    map<string, double> var;
    unsigned line = 1;

    Rule SPACE, EOL, ALPHA, ALNUM, SIGN, DIGIT, DOT, UDEC, EXP, COMM;     
    Rule WS, LPAR, RPAR, ADD, SUB, MUL, DIV, POW, EQUALS, ENDL, PRINT, IDENT, NUMBER;
    Rule calc, error, statement, expression, term, factor, atom;

public:

    calculator(istream &in = cin) : Parser(calc, in)
    {
        // Basic lexical definitions 

        SPACE       = " \t\f"_ccl;
        EOL         = ("\r\n" | "\r\n"_ccl)                 do_( ++line; );
        ALPHA       = "_a-zA-Z"_ccl;
        ALNUM       = "_a-zA-Z0-9"_ccl;
        SIGN        = "+-"_ccl;
        DIGIT       = "0-9"_ccl;
        DOT         = '.';
        UDEC        = +DIGIT >> ~(DOT >> *DIGIT) | DOT >> +DIGIT;
        EXP         = 'e' >> ~SIGN >> +DIGIT;
        COMM        = "//" >> *(!EOL >> Any());

        // Tokens

        WS          = *SPACE;
        LPAR        = '(' >> WS;
        RPAR        = ')' >> WS;
        ADD         = '+' >> WS;
        SUB         = '-' >> WS;
        MUL         = '*' >> WS;
        DIV         = '/' >> WS;
        POW         = '^' >> WS;
        EQUALS      = '=' >> WS;
        ENDL        = (~COMM >> EOL | ';') >> WS;
        PRINT       = "print" >> !ALNUM  >> WS;
        IDENT       = !PRINT >> (ALPHA >> *ALNUM)-- >> WS   do_( val(0) = text(); );
        NUMBER      = (UDEC >> ~EXP)-- >> WS                do_( val(0) = stod(text()); );

        // Calculator grammar

        calc        = WS >> ~statement >> ENDL
                    | WS >> error >> ENDL
                    ;    

        error       = (+(!ENDL >> Any()))--                 do_( cerr << "line " << line << ": ERROR: " << text() << endl; )
                    ;

        statement   = PRINT >> expression                   do_( cout << val<double>(1) << endl; )
                    | expression    
                    ;

        expression  = IDENT >> EQUALS >> expression         do_( var[val<string>(0)] = val<double>(2); val(0) = val(2); )
                    | term >> *(    
                          ADD >> term                       do_( val<double>(0) += val<double>(2); )
                        | SUB >> term                       do_( val<double>(0) -= val<double>(2); )
                        )
                    ;

        term        = factor >> *(
                          MUL >> factor                     do_( val<double>(0) *= val<double>(2); )    
                        | DIV >> factor                     do_( val<double>(0) /= val<double>(2); )
                        )
                    ;

        factor      = ADD >> factor                         do_( val(0) = val(1); )                   // unary plus
                    | SUB >> factor                         do_( val(0) = -val<double>(1); )          // unary minus
                    | atom >> *(
                          POW >> atom                       do_( val(0) = pow(val<double>(0), val<double>(2)); )
                        )
                    ;

        atom        = ADD >> atom                           do_( val(0) = val(1); )                   // unary plus
                    | SUB >> atom                           do_( val(0) = -val<double>(1); )          // unary minus
                    | NUMBER 
                    | IDENT                                 do_
                                                            (
                                                                if ( !var.count(val<string>(0)) )
                                                                    cerr << "line " << line << ": defining " << val<string>(0) << " = 0\n";
                                                                val(0) = var[val<string>(0)]; 
                                                            )
                    | LPAR >> expression >> RPAR            do_( val(0) = val(1); )
                    ;

#ifdef PEG_DEBUG

        // Rules to be debugged while checking
        peg_debug(PRINT);
        peg_debug(IDENT);
        peg_debug(EQUALS);
        peg_debug(ADD);
        peg_debug(SUB);
        peg_debug(MUL);
        peg_debug(DIV);
        peg_debug(POW);
        peg_debug(NUMBER);
        peg_debug(LPAR);
        peg_debug(RPAR);
        peg_debug(calc);
        peg_debug(error);
        peg_debug(statement);
        peg_debug(expression);
        peg_debug(term);
        peg_debug(factor);
        peg_debug(atom);

        // Check the grammar    
        check();

#endif
        
    }
};

int main()
{
    calculator c;

    // Parse and execute
    while ( c.parse() ) 
        c.accept();
}

