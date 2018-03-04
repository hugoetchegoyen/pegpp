/*

A transcription of username.leg from LEG's distribution:

%{
    #include <unistd.h>
%}

start   = "username"      { printf("%s", getlogin()); }
        | < . >           { putchar(yytext[0]); }

%%

int main()
{
    while ( yyparse() )
        ;
    return 0;
}

Note: getlogin() replaced by cuserid().

*/

#include <stdio.h>
#include <iostream>

#include "peg.h"

using namespace std;
using namespace peg;

int main()
{
    matcher m;
    Rule start;

    start   = "username"_str        ([&] { cout << cuserid(NULL); })
            | Any()--               ([&] { cout << m.text(); })
            ;

    while ( start.parse(m) )
        m.accept();
}

