#include <iostream>
#include <string>
#include <locale> 
#include <codecvt> 
#include <math.h>

#include "peg.h"
#include "json.h"

using namespace std;
using namespace peg;
using namespace json;

class JsonParser : public Parser<json_type>
{
    static string get_utf8(const string &source) 
    {
        static wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> convert;
        
        u16string s16;

        for ( unsigned i = 0 ; i < source.length() ; )
        {
            unsigned char c = source[i++];

            if ( c != '\\' )
            {
                s16 += c;
                continue;
            }

            switch ( c = source[i++] )
            {
                case '"': 
                case '\\':
                case '/':   s16 += c;      break;

                case 'b':   s16 += '\b';   break;
                case 'f':   s16 += '\f';   break;
                case 'n':   s16 += '\n';   break;
                case 'r':   s16 += '\r';   break;
                case 't':   s16 += '\t';   break;

                case 'u':   s16 += stoi(source.substr(i, 4), nullptr, 16);
                            i += 4;
                            break;
            }
        } 

        return convert.to_bytes(s16); 
    } 
 
    Rule    Eof{"Eof"}, WS, LBracket{"LBracket"}, RBracket{"RBracket"}, 
            LBrace{"LBrace"}, RBrace{"RBrace"}, Colon{"Colon"}, Comma{"Comma"}, 
            Boolean{"Boolean"}, Null{"Null"}, 
            Number{"Number"}, Sign, Whole, Fraction, Exponent, 
            String{"String"}, Char, PlainChar, EscapedChar, UTF16,
            Json, Value, Object, Array;

public:

    JsonParser(size_t tabsize, istream &in = cin) : Parser(Json, in)	
    {
        // Tokens

        Eof         =   !Any();
        WS          =   *" \t\r\n"_ccl;

        LBracket    =   '[' >> WS;
        RBracket    =   ']' >> WS;
        LBrace      =   '{' >> WS;
        RBrace      =   '}' >> WS;
        Colon       =   ':' >> WS;
        Comma       =   ',' >> WS;

        Null        =   "null" >> WS                        do_( val(0) = nullptr; )
                    ;

        Boolean     =   "true" >> WS                        do_( val(0) = true; )
                    |   "false" >> WS                       do_( val(0) = false; )
                    ;

        Number      =   ( ~Sign >> Whole >> ~Fraction >> ~Exponent )-- >> WS
                                                            do_( val(0) = stod(text()); )
                    ;
        Sign        =   '-';
        Whole       =   '0' 
                    |   "1-9"_ccl >> *"0-9"_ccl
                    ;
        Fraction    =   '.' >> +"0-9"_ccl;
        Exponent    =   "eE"_ccl >> ~"+-"_ccl >> +"0-9"_ccl;

        String      =   '"' >> ( *Char )-- >> '"' >> WS     do_( val(0) = get_utf8(text()); )
                    ;
        Char        =   PlainChar 
                    |   EscapedChar 
                    |   UTF16
                    ;
        PlainChar   =   "^\x00-\x1F\"\\"_ccl;
        EscapedChar =   '\\' >> "\"\\/bfnrt"_ccl;
        UTF16       =   "\\u" >> "0-9a-fA-F"_ccl[4];

        // Grammar

        Json        =   WS >> Value >> Eof                  do_( cout << json_formatter(tabsize).format(val(1)) << endl; )
                    ;

        Value       =   Object
                    |   Array
                    |   String
                    |   Number
                    |   Boolean
                    |   Null
                    ;

        Array       =   LBracket                            do_( val(0) = array_type(); )
                        >> ~
                        (
                            Value                           do_( val<array_type>(0).push_back(val(1)); )
                            >> *
                            (
                                Comma >> Value              do_( val<array_type>(0).push_back(val(3)); )
                            )
                        )   
                        >> RBracket
                    ;

        Object      =   LBrace                              do_( val(0) = object_type(); )
                        >> ~
                        (
                            String >> Colon >> Value        do_( val<object_type>(0)[val<string_type>(1)] = val(3); )
                            >> *
                            (
                                Comma >> String >> Colon >> Value   
                                                            do_( val<object_type>(0)[val<string_type>(5)] = val(7); )
                            )
                        )
                        >> RBrace
                    ;
    }
};


int main(int argc, char *argv[])
{
    size_t tabsize = argc > 1 ? atoi(argv[1]): 4;

    if ( tabsize > 16 )
        tabsize = 16;

    JsonParser jp(tabsize);

    // Parse and execute
    if ( jp.parse() ) 
        jp.accept();
    else
        cerr << jp.get_error() << endl;
}


