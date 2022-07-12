/*
An integer calculator with four operations and parentheses
*/

#include <iostream>
#include <string>

#include "peg.h"

using namespace std;
using namespace peg;

class intcalc : public Parser<int>
{
    Rule WS, SIGN, DIGIT, NUMBER{"NUMBER"}, LPAR{"LPAR"}, RPAR{"RPAR"};
    Rule ADD{"ADD"}, SUB{"SUB"}, MUL{"MUL"}, DIV{"DIV"};
    Rule calc, expression, term, factor;

    bool eof{false};

public:

    bool at_eof() { return eof; }

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
        calc        = WS >> (
                          (!Any())                  do_( eof = true; )
                        | expression                do_( cout << val(1) << endl; )
                        )
                    ;
        expression  = term >> *(    
                          ADD >> term               do_( val(0) += val(2); )
                        | SUB >> term               do_( val(0) -= val(2); )
                        )
                    ; 
        term        = factor >> *(
                          MUL >> factor             do_( val(0) *= val(2); )  
                        | DIV >> factor             do_( val(0) /= val(2); )
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
        if ( calc.at_eof() )
            return 0;
        else
            calc.accept();

    cerr << calc.get_error() << endl;
}



