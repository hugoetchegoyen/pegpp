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

*/

#include <unistd.h>
#include <iostream>

#include "peg.h"

using namespace std;
using namespace peg;

int main()
{
    matcher m;

    Rule start;

    start   = "username"_lit        do_( cout << getlogin(); )
            | Any()--               do_( cout << m.text(); )
            ;

    while ( start.parse(m) )
        m.accept();
}
