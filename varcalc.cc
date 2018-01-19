#include <iostream>
#include <string>
#include <map>

#define PEG_USE_SHARED_PTR

#include "peg.h"

using namespace std;
using namespace peg;

/*
    This calculator supports the four basic operations, unary plus and minus, 
    raising to a power (with ^) and grouping with parenthesis. 

    Values are of type double. Input format is a decimal number, optionally
    preceded by a + or - sign, and optionally followed by an exponent (letter "e"
    followed by a signed or unsigned integer).

    Named variables are supported. Names have the format of C identifiers. 

    Assignment is an expression:

        a = b = c = 1

    If a variable x is used before being defined, it is automatically defined as 0 
    and a warning is printed ("defining x = 0").

    The calculator accepts statements, one per line. Newline is a statement terminator.

    Statements can be:

        <expression>            (execute)
        print <expression>      (execute and print result)
    
    White space is ignored.
    Empty lines are ignored.
    Comments (everything after // up to the end of the line) are ignored. 
    Invalid lines are reported as errors and ignored. 
    Valid statements are executed.
*/

int main(int argc, char *argv[])
{
    matcher m;
    value_stack<double> val(m);
    value_stack<string> name(m);
    map<string, double> var;
    unsigned line = 1;

    // Basic lexical definitions 

    Rule SPACE  = " \t\f"_ccl;
    Rule EOL    = ("\r\n" | "\r\n"_ccl)                ([&] { ++line; });
    Rule ALPHA  = "_a-zA-Z"_ccl;
    Rule ALNUM  = "_a-zA-Z0-9"_ccl;
    Rule SIGN   = "+-"_ccl;
    Rule DIGIT  = "0-9"_ccl;
    Rule DOT    = '.';
    Rule UDEC   = +DIGIT >> ~(DOT >> *DIGIT) | DOT >> +DIGIT;
    Rule EXP    = 'e' >> ~SIGN >> +DIGIT;
    Rule COMM   = "//" >> *(!EOL >> Any());

    // Tokens

    Rule WS     = *SPACE;
    Rule LPAR   = '(' >> WS;
    Rule RPAR   = ')' >> WS;
    Rule ADD    = '+' >> WS;
    Rule SUB    = '-' >> WS;
    Rule MUL    = '*' >> WS;
    Rule DIV    = '/' >> WS;
    Rule POW    = '^' >> WS;
    Rule EQUALS = '=' >> WS;
    Rule ENDL   = ~COMM >> EOL >> WS;
    Rule PRINT  = "print" >> !ALNUM  >> WS;
    Rule IDENT  = !PRINT >> (ALPHA >> *ALNUM)-- >> WS   ([&] { name[0] = m.text(); });
    Rule NUMBER = (UDEC >> ~EXP)-- >> WS                ([&] { val[0] = stof(m.text()); });

    // Calculator grammar

    Rule calc, statement, expression, term, factor, atom;

    calc        = WS >> ~statement >> ENDL
                | WS >> (+(!ENDL >> Any()))--           ([&] { cerr << "line " << line << ": ERROR: " << m.text() << endl; })
                    >> ENDL
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
                      POW >> factor                     ([&] { val[0] = pow(val[0], val[2]); })
                    )
                ;

    atom        = NUMBER 
                | IDENT                                 ([&] 
                                                        {
                                                            if ( !var.count(name[0]) )
                                                                cerr << "line " << line << ": defining " << name[0] << " = 0\n";
                                                            val[0] = var[name[0]]; 
                                                        })
                | LPAR >> expression >> RPAR            ([&] { val[0] = val[1]; })
                ;

    // Parse and execute

    while ( calc.parse(m) ) 
        m.accept();
}

