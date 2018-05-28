// A floating point calculator supporting exponentiation and named variables.

#include <iostream>
#include <string>
#include <map>

#include <math.h>

#define PEG_USE_SHARED_PTR
// #define PEG_DEBUG     // Uncomment this for checking the grammar

#include "peg.h"

using namespace std;
using namespace peg;

int main(int argc, char *argv[])
{
    matcher m;
    value_stack<double> val(m);
    value_stack<string> name(m);
    map<string, double> var;
    unsigned line = 1;

    // Basic lexical definitions 

    Rule SPACE, EOL, ALPHA, ALNUM, SIGN, DIGIT, DOT, UDEC, EXP, COMM;     

    SPACE       = " \t\f"_ccl;
    EOL         = ("\r\n" | "\r\n"_ccl)                 _( ++line; );
    ALPHA       = "_a-zA-Z"_ccl;
    ALNUM       = "_a-zA-Z0-9"_ccl;
    SIGN        = "+-"_ccl;
    DIGIT       = "0-9"_ccl;
    DOT         = '.';
    UDEC        = +DIGIT >> ~(DOT >> *DIGIT) | DOT >> +DIGIT;
    EXP         = 'e' >> ~SIGN >> +DIGIT;
    COMM        = "//" >> *(!EOL >> Any());

    // Tokens

    Rule WS, LPAR, RPAR, ADD, SUB, MUL, DIV, POW, EQUALS, ENDL, PRINT, IDENT, NUMBER;

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
    IDENT       = !PRINT >> (ALPHA >> *ALNUM)-- >> WS   _( name[0] = m.text(); );
    NUMBER      = (UDEC >> ~EXP)-- >> WS                _( val[0] = stof(m.text()); );

    // Calculator grammar

    Rule calc, error, statement, expression, term, factor, atom;

    calc        = WS >> ~statement >> ENDL
                | WS >> error >> ENDL
                ;    

    error       = (+(!ENDL >> Any()))--                 _( cerr << "line " << line << ": ERROR: " << m.text() << endl; )
                ;

    statement   = PRINT >> expression                   _( cout << val[1] << endl; )
                | expression    
                ;

    expression  = IDENT >> EQUALS >> expression         _( val[0] = var[name[0]] = val[2]; )
                | term >> *(    
                      ADD >> term                       _( val[0] += val[2]; )
                    | SUB >> term                       _( val[0] -= val[2]; )
                    )
                ;

    term        = factor >> *(
                      MUL >> factor                     _( val[0] *= val[2]; )    
                    | DIV >> factor                     _( val[0] /= val[2]; )
                    )
                ;

    factor      = ADD >> factor                         _( val[0] = val[1]; )         // unary plus
                | SUB >> factor                         _( val[0] = -val[1]; )        // unary minus
                | atom >> *(
                      POW >> atom                       _( val[0] = pow(val[0], val[2]); )
                    )
                ;

    atom        = ADD >> atom                           _( val[0] = val[1]; )         // unary plus
                | SUB >> atom                           _( val[0] = -val[1]; )        // unary minus
                | NUMBER 
                | IDENT                                 _(
                                                            if ( !var.count(name[0]) )
                                                                cerr << "line " << line << ": defining " << name[0] << " = 0\n";
                                                            val[0] = var[name[0]]; 
                                                         )
                | LPAR >> expression >> RPAR            _( val[0] = val[1]; )
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
    calc.check();

#endif

    // Parse and execute
    while ( calc.parse(m) ) 
        m.accept();
}

