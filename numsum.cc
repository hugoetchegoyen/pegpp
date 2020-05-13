#include <iostream>
#include <string>
#include "peg.h"

using namespace std;
using namespace peg;

int main()
{
    matcher m;
    value_stack<var<int, string>> v(m);

    Rule start, sum, other, number;

    start   = sum                   do_( cout << v[0].val<int>(); )
            | other                 do_( cout << v[0].val<string>(); )
            ;
    sum     = number >> *(
                    '+' >> number   do_( v[0].val<int>() += v[2].val<int>(); )
                )
            ;
    number  = (+"0-9"_ccl)--        do_( v[0] = stoi(m.text()); )     // return int
            ;
    other   = Any()--               do_( v[0] = m.text(); )           // return string
            ;

    while ( start.parse(m) )
        m.accept();
}
