#include <string>
#include <iostream>
#include <chrono>
#include <thread>

#include "peg.h"

using namespace std;
using namespace peg;    

int main(int argc, char *argv[])
{
    static bool memoize = argc > 1;

    class pal_parser : public Parser<string>
    {
        Rule start, pal, chr{memoize};

    public:

        pal_parser(istream &in = cin) : Parser(start, in)
        {
            start   = pal                                               do_( cout << val(0) << endl; )
                    ;

            pal     = chr >> pal >> chr     if_( val(0) == val(2) )     do_( val(0) += val(1) + val(2); )
                    | chr >> chr            if_( val(0) == val(1) )     do_( val(0) += val(1); )
                    | chr 
                    ;

            chr     = Any()--               pa_( val(0) = text(); this_thread::sleep_for(100ms); )     
                                                                        do_( val(0) = text(); )
                    ;       
        }
    };

    pal_parser p;

    const auto start = chrono::steady_clock::now();
    while ( p.parse() )
        ;
    const auto end = chrono::steady_clock::now();
    cout << "Parsing time: " << (end - start) / 1ms << "ms\n";

    p.accept();
}

