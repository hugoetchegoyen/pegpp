#include <iostream>
#include <string>
#include "peg.h"

using namespace std;
using namespace peg;

class numsum : public Parser<int, string>
{
    Rule start, sum, other, number;

public:

    numsum(istream &in = cin) : Parser(start, in)
    {
        start   = sum                   do_( cout << val<int>(0); )
                | other                 do_( cout << val<string>(0); )
                ;
        sum     = number >> *(
                        '+' >> number   do_( val<int>(0) += val<int>(2); )
                    )
                ;
        number  = (+"0-9"_ccl)--        do_( val(0) = stoi(text()); )     // return int
                ;
        other   = Any()--               do_( val(0) = text(); )           // return string
                ;
    }
};

int main()
{
    numsum ns;
    while ( ns.parse() )
        ns.accept();
}
