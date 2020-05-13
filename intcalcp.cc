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

class intcalc : public Parser<int>
{
    Rule WS, SIGN, DIGIT, NUMBER, LPAR, RPAR, ADD, SUB, MUL, DIV;
    Rule calc, expression, term, factor;

public:

    intcalc(istream &in = cin ) : Parser(calc, in)
    {
        // Lexical rules
        WS          = *" \t\f\r\n"_ccl;
        SIGN        = "+-"_ccl;
        DIGIT       = "0-9"_ccl;
        NUMBER      = (~SIGN >> +DIGIT)-- >> WS     do_( val(0) = stoi(text()); );
        LPAR        = '(' >> WS;
        RPAR        = ')' >> WS;
        ADD         = '+' >> WS;
        SUB         = '-' >> WS;
        MUL         = '*' >> WS;
        DIV         = '/' >> WS;

        // Calculator
        calc        = WS >> expression              do_( cout << val<int>(1) << endl; )
                    ;
        expression  = term >> *(    
                          ADD >> term               do_( val<int>(0) += val<int>(2); )
                        | SUB >> term               do_( val<int>(0) -= val<int>(2); )
                        )
                    ; 
        term        = factor >> *(
                          MUL >> factor             do_( val<int>(0) *= val<int>(2); )  
                        | DIV >> factor             do_( val<int>(0) /= val<int>(2); )
                        )
                    ;
        factor      = NUMBER 
                    | LPAR >> expression >> RPAR    do_( val(0) = val(1); )
                    ;
    }
};

int main()
{
    intcalc calc;
    
    while ( calc.parse() )
        calc.accept();
}



