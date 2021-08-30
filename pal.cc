// A simple palindrome recognizer

#include <string>
#include <iostream>

#include "peg.h"

using namespace std;
using namespace peg;

int main()
{
    class pal_parser : public Parser<string>
    {
        Rule start, pal, chr;

    public:

        pal_parser(istream &in = cin) : Parser(start, in)
        {
            start   = pal--                 do_( cout << text() << endl; )
                    ;

            pal     = chr >> pal >> chr     if_( val(0) == val(2) ) 
                    | chr >> chr            if_( val(0) == val(1) ) 
                    | chr
                    ;

            chr     = Any()--               pa_( val(0) = text(); )  
                    ;       
        }
    };

    pal_parser p;

    while ( p.parse() )
        p.accept();
}

