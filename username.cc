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

class parser : public Parser<>
{
    Rule start;

public:

    parser(istream &in = cin) : Parser(start, in)
    {
        start   = "username"_lit        _( cout << getlogin(); )
                | Any()--               _( cout << text(); )
                ;
    }
};

int main()
{
    parser p;
    while ( p.parse() )
        p.accept();
}



