/*
An integer calculator with four operations and parentheses
*/

#include <unistd.h>
#include <iostream>
#include <iostream>
#include <string>

#include "peg.h"

using namespace std;
using namespace peg;

int main()
{
    matcher m;
    value_stack<int> v(m);

    Rule WS, SIGN, DIGIT, NUMBER, LPAR, RPAR, ADD, SUB, MUL, DIV;
    Rule calc, expression, term, factor;

   // Lexical rules
    WS          = *" \t\f\r\n"_ccl;
    SIGN        = "+-"_ccl;
    DIGIT       = "0-9"_ccl;
    NUMBER      = (~SIGN >> +DIGIT)-- >> WS     do_( v[0] = stoi(m.text()); );
    LPAR        = '(' >> WS;
    RPAR        = ')' >> WS;
    ADD         = '+' >> WS;
    SUB         = '-' >> WS;
    MUL         = '*' >> WS;
    DIV         = '/' >> WS;

    // Calculator
    calc        = WS >> expression              do_( cout << v[1] << endl; )
                ;
    expression  = term >> *(    
                      ADD >> term               do_( v[0] += v[2]; )
                    | SUB >> term               do_( v[0] -= v[2]; )
                    )
                ; 
    term        = factor >> *(
                      MUL >> factor             do_( v[0] *= v[2]; )  
                    | DIV >> factor             do_( v[0] /= v[2]; )
                    )
                ;
    factor      = NUMBER 
                | LPAR >> expression >> RPAR    do_( v[0] = v[1]; )
                ;

    while ( calc.parse(m) )
        m.accept();
}



