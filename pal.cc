// A simple palindrome recognizer

#include <string>
#include <iostream>

#include "peg.h"

using namespace std;
using namespace peg;

int main()
{
    matcher m;
    value_stack<string> v(m);

    Rule start, pal, chr;

    start   = pal--                 do_( cout << m.text() << endl; )
            ;
    pal     = chr >> pal >> chr     if_( v[0] == v[2] ) 
            | chr >> chr            if_( v[0] == v[1] ) 
            | chr
            ;
    chr     = Any()--               pa_( v[0] = m.text(); )  
            ;

    while ( start.parse(m) )
        m.accept();
}

