// A floating point calculator supporting exponentiation and named variables.

#include <iostream>
#include <string>
#include <map>

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
    EOL         = ("\r\n" | "\r\n"_ccl)                ([&] { ++line; });
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
    IDENT       = !PRINT >> (ALPHA >> *ALNUM)-- >> WS   ([&] { name[0] = m.text(); });
    NUMBER      = (UDEC >> ~EXP)-- >> WS                ([&] { val[0] = stof(m.text()); });

    // Calculator grammar

    Rule calc, error, statement, expression, term, factor, atom;

    calc        = WS >> ~statement >> ENDL
                | WS >> error >> ENDL
                ;    

    error       = (+(!ENDL >> Any()))--                 ([&] { cerr << "line " << line << ": ERROR: " << m.text() << endl; })
                ;

    statement   = PRINT >> expression                   ([&] { cout << val[1] << endl; })
                | expression    
                ;

    expression  = IDENT >> EQUALS >> expression         ([&] { val[0] = var[name[0]] = val[2]; })
                | term >> *(    
                      ADD >> term                       ([&] { val[0] += val[2]; })
                    | SUB >> term                       ([&] { val[0] -= val[2]; })
                    )
                ;

    term        = factor >> *(
                      MUL >> factor                     ([&] { val[0] *= val[2]; })    
                    | DIV >> factor                     ([&] { val[0] /= val[2]; })
                    )
                ;

    factor      = ADD >> factor                         ([&] { val[0] = val[1]; })         // unary plus
                | SUB >> factor                         ([&] { val[0] = -val[1]; })        // unary minus
                | atom >> *(
                      POW >> atom                       ([&] { val[0] = pow(val[0], val[2]); })
                    )
                ;

    atom        = ADD >> atom                           ([&] { val[0] = val[1]; })         // unary plus
                | SUB >> atom                           ([&] { val[0] = -val[1]; })        // unary minus
                | NUMBER 
                | IDENT                                 ([&] 
                                                        {
                                                            if ( !var.count(name[0]) )
                                                                cerr << "line " << line << ": defining " << name[0] << " = 0\n";
                                                            val[0] = var[name[0]]; 
                                                        })
                | LPAR >> expression >> RPAR            ([&] { val[0] = val[1]; })
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

