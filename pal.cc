/*

An example demonstrating the use of semantic predicates and parsing-time actions.
This parser splits the input stream into palindromes and prints them one per line. 
A palindrome is defined as a symmetric string of length 1 or more.

If there are no repeated characters the parser outputs the longest possible
palindromes. With repeated characters the behaviour is more erratic: it outputs 
palindromes, but not always the longest ones.

Some input -> output samples:

    abcba -> abcba

    aaaaa -> aa
             aa
             a

    aaaa  -> aaaa

    aaa   -> aa
             a

The value stack is used only during parsing to check symmetry.

*/

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

