// A simple palindrome recognizer

#include <stdio.h>
#include <iostream>

#include "peg.h"

using namespace std;
using namespace peg;

int main()
{
    matcher m;
    value_stack<string> val(m);
    Rule start, pal, chr;

    start   = pal--                                   _( cout << m.text() << endl; )
            ;

    pal     = chr >> pal >> chr                     if_( val[0] == val[2] ) 
            | chr >> chr                            if_( val[0] == val[1] ) 
            | chr
            ;

    chr     = Any()--                               pa_( val[0] = m.text(); )  
            ;

    while ( start.parse(m) )
        m.accept();
}

